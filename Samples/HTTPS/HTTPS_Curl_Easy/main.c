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

#include "epoll_timerfd_utilities.h"

static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

// Epoll and event handler file descriptors.
static int webpageDownloadTimerFd = -1;
static int epollFd = -1;

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
///     Download a web page over HTTPS protocol using cURL.
/// </summary>
static void PerformWebPageDownload(void)
{
    CURL *curlHandle = NULL;
    CURLcode res = 0;
    MemoryBlock block = {.data = NULL, .size = 0};
    char *certificatePath = NULL;

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) < 0) || !isNetworkingReady) {
        Log_Debug("\nNot doing download because there is no internet connectivity.\n");
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
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
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
        Log_Debug("\n -===- Downloaded content (%zu bytes): -===-\n", block.size);
        Log_Debug("%s\n", block.data);
    }

cleanupLabel:
    // Clean up allocated memory.
    free(block.data);
    free(certificatePath);
    // Clean up sample's cURL resources.
    curl_easy_cleanup(curlHandle);
    // Clean up cURL library's resources.
    curl_global_cleanup();
    Log_Debug("\n -===- End of download -===-\n");

exitLabel:
    return;
}

/// <summary>
///     The timer event handler.
/// </summary>
static void TimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(webpageDownloadTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    PerformWebPageDownload();
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData timerEventData = {.eventHandler = &TimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    // Issue an HTTPS request at the specified period.
    struct timespec tenSeconds = {10, 0};
    webpageDownloadTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &tenSeconds, &timerEventData, EPOLLIN);
    if (webpageDownloadTimerFd < 0) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
    // Close the timer and epoll file descriptors.
    CloseFdAndPrintError(webpageDownloadTimerFd, "WebpageDownloadTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("cURL easy interface based application starting.\n");
    Log_Debug("This sample periodically attempts to download a webpage, using curl's 'easy' API.");

    if (InitHandlers() != 0) {
        terminationRequired = true;
    } else {
        // Download the web page immediately.
        PerformWebPageDownload();
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    CloseHandlers();
    Log_Debug("Application exiting.\n");
    return 0;
}
