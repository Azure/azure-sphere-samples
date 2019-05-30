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
#include <applibs/storage.h>

#include "epoll_timerfd_utilities.h"
#include "log_utils.h"
#include "web_client.h"

/// File descriptor for the timerfd running for cURL.
static int curlTimerFd = -1;
static int epollFd = -1;

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
    {.url = "https://httpstat.us/200?sleep=5000"},
    // Download a web page with a delay of 1 second with status 400.
    {.url = "https://httpstat.us/400?sleep=1000"}};

/// cURL transfers in progress (i.e. not completed) as reported by curl_multi_socket_action().
static int runningEasyHandles = 0;
/// Time out provided by cURL.
static int curlTimeout = -1;
// The number of outstanding transfers in progress executed by cURL.
size_t curlTransferInProgress = 0;

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
/// <param name="reponse">Storage of the response</param>
static CURL *CurlSetupEasyHandle(char *url, HttpResponse *response)
{
    CURL *returnedEasyHandle = NULL; // Easy cURL handle for a transfer.
    CURLcode res = 0;
    // Certificates bundle path storage.
    char *certificatePath = NULL;

    // Create the cURL easy handle.
    CURL *easyHandle = NULL;
    if ((easyHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed.\n");
        goto errorLabel;
    }

    // Set the URL to be downloaded.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_URL, url)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto errorLabel;
    }

    // Follow redirect, i.e. 3xx statuses.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        goto errorLabel;
    }

    // Allow only HTTP and HTTPS for transfers and for redirections.
    long allowedProtocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_PROTOCOLS, allowedProtocols)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_PROTOCOLS", res);
        goto errorLabel;
    }
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_REDIR_PROTOCOLS, allowedProtocols)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_REDIR_PROTOCOLS", res);
        goto errorLabel;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION,
                                &CurlStoreDownloadedContentCallback)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEFUNCTION", res);
        goto errorLabel;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, (void *)&response->content)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEDATA", res);
        goto errorLabel;
    }

    // Set the custom parameter of the for headers retrieval.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, (void *)response)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_HEADERDATA", res);
        goto errorLabel;
    }

    // Specify a user agent.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_USERAGENT, "libcurl/1.0")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_USERAGENT", res);
        goto errorLabel;
    }

    // Get the full path to the certificates bundle file used to authenticate the HTTPS server
    // identity.
    certificatePath = Storage_GetAbsolutePathInImagePackage("certs/bundle.pem");
    if (certificatePath == NULL) {
        LogErrno("ERROR: The certificate path could not be resolved");
        goto errorLabel;
    }

    // Set the path for the certificate file that cURL uses to validate the server certificate.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_CAINFO, certificatePath)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_CAINFO", res);
        goto errorLabel;
    }

    // Turn off verbosity of cURL.
    if ((res = curl_easy_setopt(easyHandle, CURLOPT_VERBOSE, 0)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_VERBOSE", res);
        goto errorLabel;
    }

    // Set the return value on success only.
    returnedEasyHandle = easyHandle;

errorLabel:
    if (returnedEasyHandle == NULL)
        curl_easy_cleanup(easyHandle);
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
        LogCurlError("curl_multi_socket_action", code);
        return;
    }
}

/// <summary>
///     Single shot timer event handler to let cURL start the web transfers.
/// </summary>
static void CurlTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(eventData->fd) != 0) {
        Log_Debug("ERROR: cannot consume the timerfd event.\n");
        return;
    }

    CurlProcessTransfers();
}

// The context of the timerfd callback.
static EventData curlTimerEventData = {.eventHandler = &CurlTimerEventHandler};

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
            for (size_t i = 0; i < sizeof(webTransfers) / sizeof(*webTransfers); i++) {
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
                    Log_Debug("Downloaded content:\n\n%s\n",
                              webTransfers[i].httpResponse.content.data);
                    Log_Debug("End of downloaded content.\n");

                    free(webTransfers[i].httpResponse.content.data);
                    webTransfers[i].httpResponse.content.data = 0;
                    webTransfers[i].httpResponse.content.size = 0;
                }
            }
        }
    } while (curlMessage);
}

/// <summary>
///     The callback function called by upon activity on a cURL managed file descriptor.
///     This function let cURL proceed forward with the web transfers by calling
///     curl_multi_socket_action.
/// </summary>
/// <param name="eventData">Event data provided.</param>
static void CurlFdEventHandler(EventData *eventData)
{
    CURLMcode code;
    int newRunningEasyHandles = 0;
    if ((code = curl_multi_socket_action(curlMulti, eventData->fd, 0, &newRunningEasyHandles)) !=
        CURLM_OK) {
        LogCurlError("curl_multi_socket_action", code);
        return;
    }
    // Each time the 'running_handles' counter changes, curl_multi_info_read() will return info
    // about the specific transfers that completed.
    if (newRunningEasyHandles != runningEasyHandles) {
        CurlProcessCompletedTransfer();
    }
    runningEasyHandles = newRunningEasyHandles;
}

static void CurlInitCallbackData(EventData *curlData, int fd)
{
    curlData->eventHandler = CurlFdEventHandler;
    curlData->fd = fd;
}

/// <summary>
///     The socket manager callback invoked by cURL.
///     This function adds and removes socket file descriptors to the epoll set.
/// </summary>
static int CurlSocketCallback(CURL *easy, curl_socket_t fd, int action, void *u,
                              void *socketUserData)
{
    EventData *curlCallbackData = (EventData *)socketUserData;

    // The kernel could remove closed file descriptors from the epoll set,
    // hence EBADF failures are expected and ignored.
    if (action == CURL_POLL_REMOVE) {
        int res = UnregisterEventHandlerFromEpoll(epollFd, fd);
        if (res == -1) {
            Log_Debug("ERROR: Removal of event handler from epoll fd set failed.\n");
            return -1;
        }

        // Release the memory allocated for the 'fd' socket.
        free(curlCallbackData);
        return 0;
    }

    uint32_t eventsMask = 0;

    if (action == CURL_POLL_IN || action == CURL_POLL_INOUT) {
        eventsMask |= EPOLLIN;
    }
    if (action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
        eventsMask |= EPOLLOUT;
    }

    if (eventsMask != 0) {

        // Allocate memory to associate callback data to the socket's file descriptor.
        if (curlCallbackData == NULL) {
            curlCallbackData = malloc(sizeof(EventData));
            curl_multi_assign(curlMulti, fd, curlCallbackData);
        }

        // Associate the user data with the socket's file descriptor.
        CurlInitCallbackData(curlCallbackData, fd);

        int res = RegisterEventHandlerToEpoll(epollFd, fd, curlCallbackData, eventsMask);
        if (res == -1) {
            LogErrno("ERROR: Could not add or modify fd '%d' the epoll set", fd);
            return -1;
        }
    }

    return 0;
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
            SetTimerFdToSingleExpiry(curlTimerFd, &timeout);
        }
    }

    return 0;
}

/// <summary>
///     Initializes the cURL library for downloading concurrently a set of web pages.
/// </summary>
/// <returns>0 on success, -1 on error</returns>
static int CurlInit(void)
{
    CURLMcode res;

    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        Log_Debug("curl_global_init failed!\n");
        return -1;
    }
    Log_Debug("Using %s\n", curl_version());

    for (size_t i = 0; i < sizeof(webTransfers) / sizeof(*webTransfers); i++) {
        webTransfers[i].easyHandle =
            CurlSetupEasyHandle(webTransfers[i].url, &webTransfers[i].httpResponse);

        if (webTransfers[i].easyHandle == NULL) {
            goto errorLabel;
        }
    }

    // Setup cURL multi interface.
    curlMulti = curl_multi_init();
    if (curlMulti == NULL) {
        Log_Debug("curl_multi_init() failed!\n");
        goto errorLabel;
    }

    if ((res = curl_multi_setopt(curlMulti, CURLMOPT_SOCKETFUNCTION, CurlSocketCallback)) !=
        CURLM_OK) {
        LogCurlError("curl_easy_setopt CURLMOPT_SOCKETFUNCTION", res);
        goto errorLabel;
    }
    if ((res = curl_multi_setopt(curlMulti, CURLMOPT_TIMERFUNCTION, CurlTimerCallback)) !=
        CURLM_OK) {
        LogCurlError("curl_easy_setopt CURLMOPT_TIMERFUNCTION", res);
        goto errorLabel;
    }

    return 0;

errorLabel:
    for (size_t i = 0; i < sizeof(webTransfers) / sizeof(*webTransfers); i++) {
        curl_easy_cleanup(webTransfers[i].easyHandle);
    }
    return -1;
}

/// <summary>
///     Finalizes the cURL resources.
/// <summary>
static void CurlFini(void)
{
    for (size_t i = 0; i < sizeof(webTransfers) / sizeof(*webTransfers); i++) {
        curl_easy_cleanup(&webTransfers[i]);
    }

    CURLMcode res;
    if ((res = curl_multi_cleanup(curlMulti)) != CURLM_OK) {
        LogCurlError("curl_multi_cleanup failed", res);
    }
    curl_global_cleanup();
}

int WebClient_StartTransfers(void)
{
    int res = 0;
    // Start new web page downloads if not already in progress.
    if (curlTransferInProgress == 0) {
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        CURLMcode res;
        for (size_t i = 0; i < sizeof(webTransfers) / sizeof(*webTransfers); i++) {
            if ((res = curl_multi_remove_handle(curlMulti, webTransfers[i].easyHandle)) !=
                CURLM_OK) {
                LogCurlError("curl_multi_remove_handle", res);
                res = -1;
                break;
            }
            if ((res = curl_multi_add_handle(curlMulti, webTransfers[i].easyHandle)) != CURLM_OK) {
                LogCurlError("curl_multi_add_handle", res);
                res = -1;
                break;
            }
            webTransfers[i].startTime = currentTime;
            curlTransferInProgress++;
        }
    }
    return res;
}

int WebClient_Init(int epollFdInstance)
{
    epollFd = epollFdInstance;

    // By default this timer is disarmed.
    static const struct timespec curlTimerInterval = {0, 0};
    curlTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &curlTimerInterval, &curlTimerEventData, EPOLLIN);
    if (curlTimerFd < 0) {
        return -1;
    }

    return CurlInit();
}

void WebClient_Fini(void)
{
    CurlFini();
    CloseFdAndLogOnError(curlTimerFd, "curlTimerFd");
}
