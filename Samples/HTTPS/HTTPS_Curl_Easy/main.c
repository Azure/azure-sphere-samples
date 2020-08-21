/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere periodically downloads and outputs the index web page
// at example.com, by using cURL over a secure HTTPS connection.
// It uses the cURL 'easy' API which is a synchronous (blocking) API.
//
// It uses the following Azure Sphere libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging);
// - storage (device storage interaction);
// - curl (URL transfer library).
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <curl/curl.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>

#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_TimerHandler_Consume = 2,
    ExitCode_Init_EventLoop = 3,
    ExitCode_Init_DownloadTimer = 4,
    ExitCode_Main_EventLoopFail = 5,
    ExitCode_InterfaceConnectionStatus_Failed = 6
} ExitCode;

static void TerminationHandler(int signalNumber);
static size_t StoreDownloadedDataCallback(void *chunks, size_t chunkSize, size_t chunksCount,
                                          void *memoryBlock);
static void LogCurlError(const char *message, int curlErrCode);
static void PrintResponse(const char *data, size_t actualLength, size_t maxPrintLength);
static void PerformWebPageDownload(void);
static void TimerEventHandler(EventLoopTimer *timer);
static ExitCode InitHandlers(void);
static void CloseHandlers(void);
static bool IsNetworkInterfaceConnectedToInternet(void);

static EventLoop *eventLoop = NULL;
static EventLoopTimer *downloadTimer = NULL;
static const char networkInterface[] = "wlan0";

static volatile sig_atomic_t exitCode = ExitCode_Success;

// The maximum number of characters which are printed from the HTTP response body.
static const size_t maxResponseCharsToPrint = 2048;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Data pointer and size of a block of memory allocated on the heap.
/// </summary>
typedef struct {
    char *data;
    size_t size;
} MemoryBlock;

/// <summary>
///     Callback for curl_easy_perform() that copies all the downloaded chunks in a single memory
///     block.
/// <param name="chunks">The pointer to the chunks array</param>
/// <param name="chunkSize">The size of each chunk</param>
/// <param name="chunksCount">The count of the chunks</param>
/// <param name="memoryBlock">The pointer where all the downloaded chunks are aggregated</param>
/// </summary>
static size_t StoreDownloadedDataCallback(void *chunks, size_t chunkSize, size_t chunksCount,
                                          void *memoryBlock)
{
    MemoryBlock *block = (MemoryBlock *)memoryBlock;

    size_t additionalDataSize = chunkSize * chunksCount;
    block->data = realloc(block->data, block->size + additionalDataSize + 1);
    if (block->data == NULL) {
        Log_Debug("Out of memory, realloc returned NULL: errno=%d (%s)'n", errno, strerror(errno));
        abort();
    }

    memcpy(block->data + block->size, chunks, additionalDataSize);
    block->size += additionalDataSize;
    block->data[block->size] = 0; // Ensure the block of memory is null terminated.

    return additionalDataSize;
}

/// <summary>
///     Logs a cURL error.
/// </summary>
/// <param name="message">The message to print</param>
/// <param name="curlErrCode">The cURL error code to describe</param>
static void LogCurlError(const char *message, int curlErrCode)
{
    Log_Debug(message);
    Log_Debug(" (curl err=%d, '%s')\n", curlErrCode, curl_easy_strerror(curlErrCode));
}

/// <summary>
///     Checks that the interface is connected to the internet.
/// </summary>
static bool IsNetworkInterfaceConnectedToInternet(void)
{
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(networkInterface, &status) != 0) {
        if (errno != EAGAIN) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                      strerror(errno));
            exitCode = ExitCode_InterfaceConnectionStatus_Failed;
            return false;
        }
        Log_Debug("WARNING: Not doing download because the networking stack isn't ready yet.\n");
        return false;
    }

    if ((status & Networking_InterfaceConnectionStatus_ConnectedToInternet) == 0) {
        Log_Debug("WARNING: Not doing download because there is no internet connectivity.\n");
        return false;
    }

    return true;
}

/// <summary>
///     Print the response contents, truncating if required.
/// </summary>
/// <param name="data">
///     Content as null-terminated string.
/// </param>
/// <param name="actualLength">
///     Length of response in characters. Does not include null terminator.
/// </param>
/// <param name="maxPrintLength">
///     Maximum number of characters to print from response. Response is
///     truncated if <paramref name="actualLength" /> &gt;
///     <paramref name="maxPrintLength" />.
/// </param>
static void PrintResponse(const char *data, size_t actualLength, size_t maxPrintLength)
{
    if (maxPrintLength >= actualLength) {
        Log_Debug(" -===- Downloaded content (%zu bytes): -===- \n\n", actualLength);
        Log_Debug("%s\n", data);
        Log_Debug(" -===- End of downloaded content. -===- \n");
    } else {
        Log_Debug(" -===- Downloaded content (%zu bytes; displaying %zu): -===- \n\n", actualLength,
                  maxPrintLength);
        Log_Debug("%.*s\n", maxPrintLength, data);
        Log_Debug(" -===- End of partial downloaded content. -===- \n");
    }
}

/// <summary>
///     Download a web page over HTTPS protocol using cURL.
/// </summary>
static void PerformWebPageDownload(void)
{
    CURL *curlHandle = NULL;
    CURLcode res = 0;
    MemoryBlock block = {.data = NULL, .size = 0};
    char *certificatePath = NULL;

    if (IsNetworkInterfaceConnectedToInternet() == false) {
        goto exitLabel;
    }

    Log_Debug("\n -===- Starting download -===-\n");

    // Init the cURL library.
    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        LogCurlError("curl_global_init", res);
        goto exitLabel;
    }

    if ((curlHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed\n");
        goto cleanupLabel;
    }

    // Specify URL to download.
    // Important: any change in the domain name must be reflected in the AllowedConnections
    // capability in app_manifest.json.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, "https://example.com")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }

    // Set output level to verbose.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_VERBOSE", res);
        goto cleanupLabel;
    }

    // Get the full path to the certificate file used to authenticate the HTTPS server identity.
    // The DigiCertGlobalRootCA.pem file is the certificate that is used to verify the
    // server identity.
    certificatePath = Storage_GetAbsolutePathInImagePackage("certs/DigiCertGlobalRootCA.pem");
    if (certificatePath == NULL) {
        Log_Debug("The certificate path could not be resolved: errno=%d (%s)\n", errno,
                  strerror(errno));
        goto cleanupLabel;
    }

    // Set the path for the certificate file that cURL uses to validate the server certificate.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_CAINFO, certificatePath)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_CAINFO", res);
        goto cleanupLabel;
    }

    // Let cURL follow any HTTP 3xx redirects.
    // Important: any redirection to different domain names requires that domain name to be added to
    // app_manifest.json.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        goto cleanupLabel;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, StoreDownloadedDataCallback)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEFUNCTION", res);
        goto cleanupLabel;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&block)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEDATA", res);
        goto cleanupLabel;
    }

    // Specify a user agent.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_USERAGENT", res);
        goto cleanupLabel;
    }

    // Perform the download of the web page.
    if ((res = curl_easy_perform(curlHandle)) != CURLE_OK) {
        LogCurlError("curl_easy_perform", res);
    } else {
        PrintResponse(block.data, block.size, maxResponseCharsToPrint);
    }

cleanupLabel:
    // Clean up allocated memory.
    free(block.data);
    free(certificatePath);
    // Clean up sample's cURL resources.
    curl_easy_cleanup(curlHandle);
    // Clean up cURL library's resources.
    curl_global_cleanup();
    Log_Debug("\n -===- END-OF-DOWNLOAD -===-\n");

exitLabel:
    return;
}

/// <summary>
///     The timer event handler.
/// </summary>
static void TimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TimerHandler_Consume;
        return;
    }

    PerformWebPageDownload();
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Issue an HTTPS request at the specified period.
    static const struct timespec tenSeconds = {.tv_sec = 10, .tv_nsec = 0};
    downloadTimer = CreateEventLoopPeriodicTimer(eventLoop, &TimerEventHandler, &tenSeconds);
    if (downloadTimer == NULL) {
        return ExitCode_Init_DownloadTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
    DisposeEventLoopTimer(downloadTimer);
    EventLoop_Close(eventLoop);
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("cURL easy interface based application starting.\n");
    Log_Debug("This sample periodically attempts to download a webpage, using curl's 'easy' API.");

    exitCode = InitHandlers();
    if (exitCode == ExitCode_Success) {
        // Download the web page immediately.
        PerformWebPageDownload();
    }

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    CloseHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}
