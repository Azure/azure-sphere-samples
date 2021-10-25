/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/timerfd.h>

#include <curl/curl.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking_curl.h>
#include <applibs/storage.h>

#include "eventloop_timer_utilities.h"
#include "log_utils.h"
#include "web_client.h"
#include "curlmulti.h"

/// File descriptor for the timerfd running for cURL.
static EventLoopTimer *curlTimer = NULL;
static EventLoop *eventLoop = NULL; // not owned

/// <summary>
///     Data pointer and size of a block of memory allocated on the heap.
/// </summary>
typedef struct {
    uint8_t *data;
    size_t size;
} MemoryBlock;

/// <summary>
///     The storage for an HTTP response content.
/// </summary>
typedef struct {
    MemoryBlock content;
} HttpResponse;

// The cURL's 'multi' interface instance.
static CURLM *curlMulti = 0;

// Data type containing data for each web transfer.
typedef struct {
    CURL *easyHandle;
    char *url;
    HttpResponse httpResponse;
    struct timespec startTime;
} WebTransfer;

// The web transfers executed with cURL.
WebTransfer webTransfers[] = {
    // Download a web page with a delay of 5 seconds with status 200.
    {.url = "https://httpstat.us/200?sleep=5000", .easyHandle = NULL},
    // Download a web page with a delay of 1 second with status 400.
    {.url = "https://httpstat.us/400?sleep=1000", .easyHandle = NULL}};
static const size_t transferCount = sizeof(webTransfers) / sizeof(*webTransfers);

/// cURL transfers in progress (i.e. not completed) as reported by curl_multi_socket_action().
static int runningEasyHandles = 0;
/// Time out provided by cURL.
static int curlTimeout = -1;
// The number of outstanding transfers in progress executed by cURL.
size_t curlTransferInProgress = 0;
// The maximum number of characters which are printed from the HTTP response body.
static const size_t maxResponseCharsToPrint = 2048;

/// <summary>
///     Logs a cURL easy error.
/// </summary>
/// <param name="message">The message to print</param>
/// <param name="curlErrCode">The cURL error code to describe</param>
static void LogCurlEasyError(const char *message, CURLcode code)
{
    Log_Debug(message);
    Log_Debug(" (curl easy err=%d, '%s')\n", code, curl_easy_strerror(code));
}

/// <summary>
///     Logs a cURL multi error.
/// </summary>
/// <param name="message">The message to print</param>
/// <param name="curlErrCode">The cURL error code to describe</param>
static void LogCurlMultiError(const char *message, CURLMcode code)
{
    Log_Debug(message);
    Log_Debug(" (curl multi err=%d, '%s')\n", code, curl_multi_strerror(code));
}

/// <summary>
///     cURL callback that aggregates all the downloaded chunks in a single
///     memory block.
/// </summary>
/// <param name="chunks">The pointer to the chunks array</param>
/// <param name="chunkSize">The size of each chunk</param>
/// <param name="chunksCount">The count of the chunks</param>
/// <param name="memoryBlock">The pointer where all the downloaded chunks are aggregated</param>
static size_t CurlStoreDownloadedContentCallback(void *chunks, size_t chunkSize, size_t chunksCount,
                                                 void *userData)
{
    MemoryBlock *block = (MemoryBlock *)userData;
    size_t additionalDataSize = chunkSize * chunksCount;
    block->data = realloc(block->data, block->size + additionalDataSize + 1);
    if (block->data == NULL) {
        LogErrno("ERROR: Out of memory, realloc returned NULL");
        abort();
    }
    memcpy(block->data + block->size, chunks, additionalDataSize);
    block->size += additionalDataSize;
    block->data[block->size] = 0; // Ensure the block of memory is null terminated.
    return additionalDataSize;
}

/// <summary>
///     Creates an cURL easy handle to download the specified URL.
///     Note that:
///         - download is restricted to HTTP and HTTPS protocols only;
///         - redirects are followed;
///         - it is necessary to update the AllowedConnection's hostnames
///           in app_manifest.json.
/// </summary>
/// <param name="url">HTTP or HTTPS URL to download</param>
/// <param name="response">Storage of the response</param>
/// <param name="callerExitCode">
///     Set to ExitCode_Success if succeeded, in which case the return value is non-NULL.
///     Otherwise set to another exit code which indicates the specific failure.
/// </param>
/// <param name="bypassProxy">Boolean which indicates if proxy needs to be bypassed.</param>
/// <returns>
///     Pointer to a curl easy handle, which must be disposed of with curl_easy_cleanup.
///     On failure, returns NULL and puts a specific failure reason in *status.
/// </returns>
static CURL *CurlSetupEasyHandle(char *url, HttpResponse *response, ExitCode *callerExitCode,
                                 bool bypassProxy)
{
    CURL *returnedEasyHandle = NULL; // Easy cURL handle for a transfer.
    CURLcode res = 0;
    // Certificates bundle path storage.
    char *certificatePath = NULL;

    // Create the cURL easy handle.
    CURL *easyHandle = NULL;
    if ((easyHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed.\n");
        *callerExitCode = ExitCode_CurlSetupEasy_EasyInit;
        goto errorLabel;
    }

    // Set the URL to be downloaded.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_URL, url)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_URL", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptUrl;
        goto errorLabel;
    }

    // Follow redirect, i.e. 3xx statuses.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptFollowLocation;
        goto errorLabel;
    }

    // Allow only HTTP and HTTPS for transfers and for redirections.
    long allowedProtocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_PROTOCOLS, allowedProtocols)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_PROTOCOLS", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptProtocols;
        goto errorLabel;
    }
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_REDIR_PROTOCOLS, allowedProtocols)) !=
        CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_REDIR_PROTOCOLS", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptRedirProtocols;
        goto errorLabel;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION,
                                &CurlStoreDownloadedContentCallback)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_WRITEFUNCTION", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptWriteFunction;
        goto errorLabel;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, (void *)&response->content)) !=
        CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_WRITEDATA", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptWriteData;
        goto errorLabel;
    }

    // Set the custom parameter of the for headers retrieval.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, (void *)response)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_HEADERDATA", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptHeaderData;
        goto errorLabel;
    }

    // Specify a user agent.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_USERAGENT, "libcurl/1.0")) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_USERAGENT", res);
        *callerExitCode = ExitCode_CurlSetupEasy_OptUserAgent;
        goto errorLabel;
    }

    // Get the full path to the certificates bundle file used to authenticate the HTTPS server
    // identity.
    certificatePath = Storage_GetAbsolutePathInImagePackage("certs/bundle.pem");
    if (certificatePath == NULL) {
        LogErrno("ERROR: The certificate path could not be resolved");
        *callerExitCode = ExitCode_CurlSetupEasy_StoragePath;
        goto errorLabel;
    }

    // Set the path for the certificate file that cURL uses to validate the server certificate.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_CAINFO, certificatePath)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_CAINFO", res);
        *callerExitCode = ExitCode_CurlSetupEasy_CAInfo;
        goto errorLabel;
    }

    // Turn off verbosity of cURL.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_VERBOSE, 0)) != CURLE_OK) {
        LogCurlEasyError("curl_easy_setopt CURLOPT_VERBOSE", res);
        *callerExitCode = ExitCode_CurlSetupEasy_Verbose;
        goto errorLabel;
    }

    // Configure the curl handle to use the proxy.
    if (!bypassProxy) {
        if (Networking_Curl_SetDefaultProxy(easyHandle) != 0) {
            Log_Debug("Networking_Curl_SetDefaultProxy failed: errno=%d (%s)\n", errno,
                      strerror(errno));
            *callerExitCode = ExitCode_CurlSetupEasy_CurlSetDefaultProxy;
            goto errorLabel;
        }
    }

    // When using libcurl, as with other networking applications, the Azure Sphere OS will
    // allocate socket buffers which are attributed to your application's RAM usage. You can tune
    // the size of these buffers to reduce the RAM footprint of your application as appropriate.
    // Refer to https://docs.microsoft.com/azure-sphere/app-development/ram-usage-best-practices
    // for further details.

    *callerExitCode = ExitCode_Success;
    returnedEasyHandle = easyHandle;

errorLabel:
    if (returnedEasyHandle == NULL) {
        curl_easy_cleanup(easyHandle);
    }

    free(certificatePath);
    return returnedEasyHandle;
}

/// <summary>
///     Notify cURL to start the transfers.
/// </summary>
static void CurlProcessTransfers(void)
{
    CURLMcode code;
    if ((code = curl_multi_socket_action(curlMulti, CURL_SOCKET_TIMEOUT, 0, &runningEasyHandles)) !=
        CURLM_OK) {
        LogCurlMultiError("curl_multi_socket_action", code);
        return;
    }
}

/// <summary>
///     Single shot timer event handler to let cURL start the web transfers.
/// </summary>
static void CurlTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: cannot consume the timer event.\n");
        return;
    }

    CurlProcessTransfers();
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
///     Process a completed web transfer by display HTTP status and content of the transfer.
/// </summary>
static void CurlProcessCompletedTransfer(void)
{
    struct CURLMsg *curlMessage = NULL;
    do {
        int msgq = 0;
        curlMessage = curl_multi_info_read(curlMulti, &msgq);
        if ((curlMessage != NULL) && (curlMessage->msg == CURLMSG_DONE)) {
            curlTransferInProgress--;
            CURL *e = curlMessage->easy_handle;
            for (size_t i = 0; i < transferCount; i++) {
                if (webTransfers[i].easyHandle == e) {
                    struct timespec currentTime;
                    clock_gettime(CLOCK_MONOTONIC, &currentTime);
                    // Display the HTTP status header and the content of the completed web transfer.
                    Log_Debug(
                        "\n -==- %s download complete (elapsed time %ld milliseconds) -==-\n",
                        webTransfers[i].url,
                        // Compute the elapsed time in milliseconds out of the timespecs.
                        (currentTime.tv_sec - webTransfers[i].startTime.tv_sec) * 1000 +
                            (currentTime.tv_nsec - webTransfers[i].startTime.tv_nsec) / 1000000);

                    PrintResponse(webTransfers[i].httpResponse.content.data,
                                  webTransfers[i].httpResponse.content.size,
                                  maxResponseCharsToPrint);

                    free(webTransfers[i].httpResponse.content.data);
                    webTransfers[i].httpResponse.content.data = 0;
                    webTransfers[i].httpResponse.content.size = 0;
                }
            }
        }
    } while (curlMessage);
}

/// <summary>
///     This function is invoked when activity occurs on a cURL-managed file descriptor.
///     See <see cref="EventLoopIoCallback" /> for parameter descriptions.
/// </summary>
static void CurlFdEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    CURLMcode code;
    int newRunningEasyHandles = 0;
    if ((code = curl_multi_socket_action(curlMulti, fd, 0, &newRunningEasyHandles)) != CURLM_OK) {
        LogCurlMultiError("curl_multi_socket_action", code);
        return;
    }
    // Each time the 'running_handles' counter changes, curl_multi_info_read() will return info
    // about the specific transfers that completed.
    if (newRunningEasyHandles != runningEasyHandles) {
        CurlProcessCompletedTransfer();
    }
    runningEasyHandles = newRunningEasyHandles;
}

/// <summary>
///     The socket manager callback invoked by cURL.
///     This function sets up a notification to hear when data is read from, or
///     written to the supplied file descriptor.
/// </summary>
static int CurlSocketCallback(CURL *easy, curl_socket_t fd, int action, void *u,
                              void *socketUserData)
{
    EventRegistration *socketEvent = (EventRegistration *)socketUserData;

    // The kernel could remove closed file descriptors from the event set,
    // hence EBADF failures are expected and ignored.
    if (action == CURL_POLL_REMOVE) {
        int res = EventLoop_UnregisterIo(eventLoop, socketEvent);

        if (res == -1 && errno != EBADF) {
            Log_Debug("ERROR: Cannot unregister IO event: %d\n", errno);
        }

        return CURLM_OK;
    }

    EventLoop_IoEvents eventsMask = 0;

    if (action == CURL_POLL_IN || action == CURL_POLL_INOUT) {
        eventsMask |= EventLoop_Input;
    }
    if (action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
        eventsMask |= EventLoop_Output;
    }

    if (socketEvent == NULL) {
        socketEvent = EventLoop_RegisterIo(eventLoop, fd, 0x0, CurlFdEventHandler, NULL);

        if (socketEvent == NULL) {
            LogErrno("ERROR: Could not create socket event", errno);
            return -1;
        }

        curl_multi_assign(curlMulti, fd, socketEvent);
    }

    int res = EventLoop_ModifyIoEvents(eventLoop, socketEvent, eventsMask);
    if (res == -1) {
        LogErrno("ERROR: Could not add or modify socket event mask", fd);
        return -1;
    }

    return CURLM_OK;
}

/// <summary>
///     The callback invoked by cURL to report the period of time time within the
///     curl_multi_socket_action() must be invoked.
/// </summary>
/// <param name="multi">The cURL multi instance handle</param>
/// <param name="timeoutMillis">The timeout expressed in milliseconds</param>
static int CurlTimerCallback(CURLM *multi, long timeoutMillis, void *unused)
{
    curlTimeout = timeoutMillis;

    // A value of -1 means the timer does not need to be started.
    if (timeoutMillis != -1) {
        // Invoke cURL immediately if requested to do so.
        if (timeoutMillis == 0) {
            CurlProcessTransfers();
        } else {
            // Start a single shot timer with the period as provided by cURL.
            // The timer handler will invoke cURL to process the web transfers.
            const struct timespec timeout = {.tv_sec = timeoutMillis / 1000,
                                             .tv_nsec = (timeoutMillis % 1000) * 1000000};
            SetEventLoopTimerOneShot(curlTimer, &timeout);
        }
    }

    return 0;
}

/// <summary>
///     Initializes the cURL library for downloading concurrently a set of web pages.
/// </summary>
/// <param name="bypassProxy">Boolean which indicates if proxy needs to be bypassed.</param>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode CurlInit(bool bypassProxy)
{
    CURLMcode res;

    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        Log_Debug("curl_global_init failed!\n");
        return ExitCode_CurlInit_GlobalInit;
    }
    Log_Debug("Using %s\n", curl_version());

    ExitCode localExitCode;

    for (size_t i = 0; i < transferCount; i++) {
        webTransfers[i].easyHandle = CurlSetupEasyHandle(
            webTransfers[i].url, &webTransfers[i].httpResponse, &localExitCode, bypassProxy);

        if (webTransfers[i].easyHandle == NULL) {
            goto errorLabel;
        }
    }

    // Setup cURL multi interface.
    curlMulti = curl_multi_init();
    if (curlMulti == NULL) {
        Log_Debug("curl_multi_init() failed!\n");
        localExitCode = ExitCode_CurlInit_MultiInit;
        goto errorLabel;
    }

    if ((res = curl_multi_setopt(curlMulti, CURLMOPT_SOCKETFUNCTION, CurlSocketCallback)) !=
        CURLM_OK) {
        LogCurlMultiError("curl_easy_setopt CURLMOPT_SOCKETFUNCTION", res);
        localExitCode = ExitCode_CurlInit_MultiSetOptSocketFunction;
        goto errorLabel;
    }
    if ((res = curl_multi_setopt(curlMulti, CURLMOPT_TIMERFUNCTION, CurlTimerCallback)) !=
        CURLM_OK) {
        LogCurlMultiError("curl_easy_setopt CURLMOPT_TIMERFUNCTION", res);
        localExitCode = ExitCode_CurlInit_MultiSetOptTimerFunction;
        goto errorLabel;
    }

    return ExitCode_Success;

errorLabel:
    for (size_t i = 0; i < transferCount; i++) {
        // Safe to call curl_easy_cleanup with NULL pointer.
        curl_easy_cleanup(webTransfers[i].easyHandle);
        // Set pointer to NULL so not cleaned up again in CurlFini.
        webTransfers[i].easyHandle = NULL;
    }
    return localExitCode;
}

/// <summary>
///     Finalizes the cURL resources.
/// <summary>
static void CurlFini(void)
{
    for (size_t i = 0; i < transferCount; i++) {
        curl_easy_cleanup(webTransfers[i].easyHandle);
    }

    CURLMcode res;
    if ((res = curl_multi_cleanup(curlMulti)) != CURLM_OK) {
        LogCurlMultiError("curl_multi_cleanup failed", res);
    }
    curl_global_cleanup();
}

int WebClient_StartTransfers(void)
{
    // Start new web page downloads if not already in progress.
    if (curlTransferInProgress == 0) {
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        CURLMcode res;
        for (size_t i = 0; i < transferCount; i++) {
            if ((res = curl_multi_remove_handle(curlMulti, webTransfers[i].easyHandle)) !=
                CURLM_OK) {
                LogCurlMultiError("curl_multi_remove_handle", res);
                return -1;
            }
            if ((res = curl_multi_add_handle(curlMulti, webTransfers[i].easyHandle)) != CURLM_OK) {
                LogCurlMultiError("curl_multi_add_handle", res);
                return -1;
            }
            webTransfers[i].startTime = currentTime;
            curlTransferInProgress++;
        }
    }

    return 0;
}

ExitCode WebClient_Init(EventLoop *eventLoopInstance, bool bypassProxy)
{
    eventLoop = eventLoopInstance;

    curlTimer = CreateEventLoopDisarmedTimer(eventLoop, &CurlTimerEventHandler);
    if (curlTimer == NULL) {
        return ExitCode_WebClientInit_CurlTimer;
    }

    return CurlInit(bypassProxy);
}

void WebClient_Fini(void)
{
    CurlFini();
    DisposeEventLoopTimer(curlTimer);
}
