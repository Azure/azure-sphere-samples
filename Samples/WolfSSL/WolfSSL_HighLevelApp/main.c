/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample uses the wolfSSL APIs to read a web page over HTTPS.
//
// It uses the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for timer events and IO callbacks)
// - networking (network ready)
// - storage (device storage interaction)
// - wolfssl (handles TLS handshake)

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <wolfssl/ssl.h>

#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>

#include "eventloop_timer_utilities.h"

/// <summary>
///     Exit codes for this application. These are used for the
///     application exit code. They must all be between zero and 255,
///     where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_IsNetworkingReady_Failed = 1,

    ExitCode_NetworkReadyCheckHandler_Consume = 2,

    ExitCode_ConnectRaw_GetHostByName = 3,
    ExitCode_ConnectRaw_Socket = 4,
    ExitCode_ConnectRaw_EventReg = 5,
    ExitCode_ConnectRaw_Connect = 6,

    ExitCode_HandleConnection_Failed = 7,
    ExitCode_HandleConnection_Init = 8,
    ExitCode_HandleConnection_Method = 9,
    ExitCode_HandleConnection_Context = 10,
    ExitCode_HandleConnection_CertPath = 11,
    ExitCode_HandleConnection_VerifyLocations = 12,
    ExitCode_HandleConnection_Session = 13,
    ExitCode_HandleConnection_CheckDomainName = 14,
    ExitCode_HandleConnection_SetFd = 15,

    ExitCode_SslHandshake_ModifyEvents = 16,
    ExitCode_SslHandshake_Fail = 17,

    ExitCode_WriteData_ModifyEventsNone = 18,
    ExitCode_WriteData_ModifyEventsOutput = 19,
    ExitCode_WriteData_Write = 20,

    ExitCode_ReadData_ModifyEventsNone = 21,
    ExitCode_ReadData_Read = 22,
    ExitCode_ReadData_Finished = 23,
    ExitCode_ReadData_ModifyEventsInput = 24,

    ExitCode_Init_EventLoop = 25,
    ExitCode_Init_NetworkReadyCheckTimer = 26,

    ExitCode_Main_EventLoopFail = 27
} ExitCode;

static volatile ExitCode exitCode = ExitCode_Success;
_Static_assert(sizeof(ExitCode) <= sizeof(sig_atomic_t), "ExitCode is larger than sig_atomic_t.");

// Notifications for internet check timer and IO events.
static EventLoop *eventLoop = NULL;
static EventLoopTimer *networkReadyCheckTimer = NULL;
static EventRegistration *sockReg = NULL;

// Function to run the next time an IO event occurs.
static void (*nextHandler)(void);

// Server which hosts the required web page. This is a macro rather than a const char*
// to simplify constructing the HTTP/1.1 request, which must have a Connection header.
// The host name must appear in the AllowedConnections capability in app_manifest.json.
#define SERVER_NAME "example.com"
static const uint16_t PORT_NUM = 443;
static const char certPath[] = "certs/DigiCertGlobalRootCA.pem";

static bool wolfSslInitialized = false;
static WOLFSSL_CTX *wolfSslCtx = NULL;
static WOLFSSL *wolfSslSession = NULL;
static int sockFd = -1;

static const uint8_t *writePayload = NULL;
static int writePayloadLen = 0;
static int totalBytesWritten = 0;
static uint8_t readPayload[16];
static int totalBytesRead = 0;

static bool IsNetworkReady(void);
static void NetworkReadyTimerEventHandler(EventLoopTimer *timer);
static void HandleSockEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static ExitCode ConnectRawSocketToServer(void);
static void HandleConnection(void);
static void HandleTlsHandshake(void);
static void WriteData(void);
static void ReadData(void);
static ExitCode InitializeResources(void);
static void FreeResources(void);

/// <summary>
///     Checks whether the network is ready.
///     If a fatal error occurs, sets exitCode and returns false.
/// </summary>
/// <returns>true if connected to the internet; false otherwise.</returns>
static bool IsNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_IsNetworkingReady_Failed;
        return false;
    }

    if (!isNetworkReady) {
        Log_Debug("WARNING: Not doing download because the network is not ready.\n");
    }

    return isNetworkReady;
}

/// <summary>
///     <para>
///         This handler is called periodically when the program starts
///         to check whether the network is ready. Once connected,
///         the timer is disarmed.  If a fatal error occurs, sets exitCode
///         to the appropriate value.
///     </para>
///     <para>
///         See <see cref="EventLoopTimerHandler" /> for more information
///         and a description of the argument.
///     </para>
/// </summary>
static void NetworkReadyTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_NetworkReadyCheckHandler_Consume;
        return;
    }

    bool internetReady = IsNetworkReady();
    if (internetReady) {
        DisarmEventLoopTimer(timer);
        exitCode = ConnectRawSocketToServer();
    }
}

/// <summary>
///     <para>
///         This function is called from the event loop when a read or write event occurs
///         on the underlying socket. It calls the function whose address is in nextHandler.
///     </para>
///     <para>
///         See <see cref="EventLoopIoCallback" /> for a description of the arguments.
///      </para>
/// </summary>
static void HandleSockEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    nextHandler();
}

/// <summary>
///     <para>
///         Open an AF_INET socket and starts an asynchronous connection
///         to the server's HTTPS port.
///     </para>
///     <para>
///         <see cref="HandleConnection" /> is called when the connection completes,
///         succesfully or otherwise.
///     </para>
/// </summary>
/// <returns>ExitCode_Success on success; another ExitCode on failure.</returns>
static ExitCode ConnectRawSocketToServer(void)
{
    static const int MaxResolveAttempts = 5;
    int resolveAttempt = 1;
    struct hostent *hent = NULL;
    while (hent == NULL && resolveAttempt <= MaxResolveAttempts) {
        hent = gethostbyname(SERVER_NAME);
        if (hent == NULL) {
            if (h_errno == TRY_AGAIN) {
                Log_Debug("gethostbyname(): TRY_AGAIN\n");
                sleep(1);
                resolveAttempt++;
            } else {
                return ExitCode_ConnectRaw_GetHostByName;
            }
        }
    }

    if (hent == NULL) {
        return ExitCode_ConnectRaw_GetHostByName;
    }

    sockFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockFd == -1) {
        return ExitCode_ConnectRaw_Socket;
    }

    sockReg = EventLoop_RegisterIo(eventLoop, sockFd, EventLoop_Output, HandleSockEvent,
                                   /* context */ NULL);
    if (sockReg == NULL) {
        return ExitCode_ConnectRaw_EventReg;
    }

    struct sockaddr_in host = {.sin_family = AF_INET,
                               .sin_port = htons(PORT_NUM),
                               .sin_addr = *((struct in_addr *)hent->h_addr)};

    int r = connect(sockFd, (const struct sockaddr *)&host, sizeof(host));
    if (r != 0 && errno != EINPROGRESS) {
        return ExitCode_ConnectRaw_Connect;
    }

    nextHandler = HandleConnection;
    return ExitCode_Success;
}

/// <summary>
///     <para>
///         Called from the event loop when socket connection has completed,
///         successfully or otherwise. If the connection was successful, then
///         uses wolfSSL to start the SSL handshake. Otherwise, set exitCode to
///         the appropriate value.
///     </para>
/// </summary>
static void HandleConnection(void)
{
    // Check whether the connection succeeded.
    int error;
    socklen_t errSize = sizeof(error);
    int r = getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &error, &errSize);
    if (!(r == 0 && error == 0)) {
        exitCode = ExitCode_HandleConnection_Failed;
        return;
    }

    // Connection was made successfully, so allocate wolfSSL session and context.
    r = wolfSSL_Init();
    if (r != WOLFSSL_SUCCESS) {
        exitCode = ExitCode_HandleConnection_Init;
        return;
    }
    wolfSslInitialized = true;

    WOLFSSL_METHOD *wolfSslMethod = wolfTLSv1_3_client_method();
    if (wolfSslMethod == NULL) {
        exitCode = ExitCode_HandleConnection_Method;
        return;
    }

    wolfSslCtx = wolfSSL_CTX_new(wolfSslMethod);
    if (wolfSslCtx == NULL) {
        exitCode = ExitCode_HandleConnection_Context;
        return;
    }

    // Specify the root certificate which is used to validate the server.
    char *certPathAbs = Storage_GetAbsolutePathInImagePackage(certPath);
    if (certPathAbs == NULL) {
        exitCode = ExitCode_HandleConnection_CertPath;
        return;
    }

    r = wolfSSL_CTX_load_verify_locations(wolfSslCtx, certPathAbs, NULL);
    free(certPathAbs);
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_CTX_load_verify_locations %d\n", r);
        exitCode = ExitCode_HandleConnection_VerifyLocations;
    }

    wolfSslSession = wolfSSL_new(wolfSslCtx);
    if (wolfSslSession == NULL) {
        exitCode = ExitCode_HandleConnection_Session;
        return;
    }

    // Check domain name of peer certificate.
    r = wolfSSL_check_domain_name(wolfSslSession, SERVER_NAME);
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_check_domain_name %d\n", r);
        exitCode = ExitCode_HandleConnection_CheckDomainName;
        return;
    }

    // Associate socket with wolfSSL session.
    r = wolfSSL_set_fd(wolfSslSession, sockFd);
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_set_fd %d\n", r);
        exitCode = ExitCode_HandleConnection_SetFd;
        return;
    }

    // Perform TLS handshake.
    // Asynchronous handshakes require repeated calls to wolfSSL_connect, so jump to the
    // handler to avoid repeating code.
    HandleTlsHandshake();
}

/// <summary>
///     <para>
///         Called to start the TLS handshake. When an IO event occurs, the event loop
///         calls this function again to check whether the handshake has completed.
///     </para>
///     <para>
///         If the handshake completes successfully, this function begins writing the
///         HTTP GET request. If a fatal error occurs, sets exitCode to the appropriate value.
///     </para>
/// </summary>
static void HandleTlsHandshake(void)
{
    int r = EventLoop_ModifyIoEvents(eventLoop, sockReg, EventLoop_Input | EventLoop_Output);
    if (r != 0) {
        exitCode = ExitCode_SslHandshake_ModifyEvents;
        return;
    }

    r = wolfSSL_connect(wolfSslSession);
    if (r != WOLFSSL_SUCCESS) {
        // If the handshake is in progress, exit to the event loop.
        const int uniqueError = wolfSSL_get_error(wolfSslSession, r);
        if (uniqueError == WOLFSSL_ERROR_WANT_READ || uniqueError == WOLFSSL_ERROR_WANT_WRITE) {
            nextHandler = HandleTlsHandshake;
            return;
        }

        // Unexpected error, so terminate.
        Log_Debug("ERROR: wolfSSL_connect %d\n", uniqueError);
        exitCode = ExitCode_SslHandshake_Fail;
        return;
    }

    // "Connection: close" instructs the server to close the connection after the
    // web page has been transferred, so this client knows when to stop reading data.
    writePayload =
        "GET / HTTP/1.1\r\n"
        "Host: " SERVER_NAME
        "\r\n"
        "Connection: close\r\n"
        "Accept: */*\r\n"
        "\r\n";
    writePayloadLen = (int)strlen(writePayload);
    totalBytesWritten = 0;

    WriteData();
}

/// <summary>
///     <para>
///         Called to start writing the HTTP GET request. If the entire request
///         could not be written in one write operation, this function is called again
///         from the event loop to write the next chunk of data.
///     </para>
///     <para>
///         Once the whole request has been written, this function starts reading the
///         response. If a fatal error occurs, sets exitCode to the appropriate value.
///     </para>
/// </summary>
static void WriteData(void)
{
    int r = EventLoop_ModifyIoEvents(eventLoop, sockReg, EventLoop_None);
    if (r != 0) {
        exitCode = ExitCode_WriteData_ModifyEventsNone;
        return;
    }

    while (totalBytesWritten < writePayloadLen) {
        int bytesRemaining = writePayloadLen - totalBytesWritten;
        int bytesWritten =
            wolfSSL_write(wolfSslSession, &writePayload[totalBytesWritten], bytesRemaining);

        if (bytesWritten <= 0) {
            bool wasFatalError = (bytesWritten == WOLFSSL_FATAL_ERROR);
            const int uniqueError = wolfSSL_get_error(wolfSslSession, bytesWritten);

            if (wasFatalError && uniqueError == WOLFSSL_ERROR_WANT_WRITE) {
                r = EventLoop_ModifyIoEvents(eventLoop, sockReg, EventLoop_Output);
                if (r != 0) {
                    exitCode = ExitCode_WriteData_ModifyEventsOutput;
                } else {
                    nextHandler = WriteData;
                }

                return;
            }

            // Unexpected error, so terminate.
            Log_Debug("ERROR: wolfSSL_write %d\n", uniqueError);
            exitCode = ExitCode_WriteData_Write;
            return;
        }

        totalBytesWritten += bytesWritten;
    }

    // Full payload has been written, so read response.
    totalBytesRead = 0;
    return ReadData();
}

/// <summary>
///     <para>
///         Called to start reading a response from the server. If the entire response
///         could not be read in one operation, this function is called again from the
///         event loop to read the next chunk of data.
///     </para>
///     <para>
///         Once the entire response has been read, or when an error occurs, exitCode is
///         set to the appropriate value, which causes control to return to the main function.
///     </para>
/// </summary>
static void ReadData(void)
{
    int r = EventLoop_ModifyIoEvents(eventLoop, sockReg, EventLoop_None);
    if (r != 0) {
        exitCode = ExitCode_ReadData_ModifyEventsNone;
        return;
    }

    int bytesRead = wolfSSL_read(wolfSslSession, readPayload, sizeof(readPayload));

    // If error occurred then abort.
    if (bytesRead <= 0) {
        bool wasFatalError = (bytesRead == WOLFSSL_FATAL_ERROR);
        const int uniqueError = wolfSSL_get_error(wolfSslSession, bytesRead);

        if (wasFatalError && uniqueError == WOLFSSL_ERROR_WANT_READ) {
            goto read_more_data;
        }

        // HTTPS connection was opened with "Connection: close" so expect the
        // server to close the connection when the transfer has completed.

        static const int SOCKET_PEER_CLOSED_E = -397;
        if (bytesRead == 0 &&
            (uniqueError == SOCKET_PEER_CLOSED_E || uniqueError == WOLFSSL_ERROR_ZERO_RETURN)) {
            exitCode = ExitCode_ReadData_Finished;
            return;
        }

        Log_Debug("ERROR: wolfSSL_read %d\n", uniqueError);
        exitCode = ExitCode_ReadData_Read;
        return;
    }

    Log_Debug("%.*s", bytesRead, readPayload);
    totalBytesRead += bytesRead;

read_more_data:
    r = EventLoop_ModifyIoEvents(eventLoop, sockReg, EventLoop_Input);
    if (r != 0) {
        exitCode = ExitCode_ReadData_ModifyEventsInput;
    } else {
        nextHandler = ReadData;
    }
}

/// <summary>
///     Allocate resources which are needed at startup, namely the
///     event loop and the startup timer.
/// </summary>
/// <returns>
///     ExitCode_Success on success; or another ExitCode value on failure.
/// </returns>
static ExitCode InitializeResources(void)
{
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        return ExitCode_Init_EventLoop;
    }

    // Check if network is ready every 10 seconds.
    static const struct timespec tenSeconds = {.tv_sec = 10, .tv_nsec = 0};
    networkReadyCheckTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &NetworkReadyTimerEventHandler, &tenSeconds);
    if (networkReadyCheckTimer == NULL) {
        return ExitCode_Init_NetworkReadyCheckTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     <para>
///         Free any resources which were successfully allocated by the program.
///         This includes the event loop, startup timer, wolfSSL resources, and socket.
///     </para>
/// </summary>
static void FreeResources(void)
{
    if (wolfSslSession != NULL) {
        wolfSSL_free(wolfSslSession);
    }

    if (wolfSslCtx != NULL) {
        wolfSSL_CTX_free(wolfSslCtx);
    }

    if (wolfSslInitialized) {
        wolfSSL_Cleanup();
    }

    if (sockFd != -1) {
        close(sockFd);
    }

    DisposeEventLoopTimer(networkReadyCheckTimer);
    EventLoop_UnregisterIo(eventLoop, sockReg);
    EventLoop_Close(eventLoop);
}

int main(void)
{
    Log_Debug("Use a socket with wolfSSL to download page over HTTPS.\n");
    Log_Debug("Connecting to %s.\n", SERVER_NAME);

    exitCode = InitializeResources();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens.
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    FreeResources();

    if (exitCode == ExitCode_ReadData_Finished) {
        Log_Debug("\nDownloaded content (%d bytes).\n", totalBytesRead);
        exitCode = ExitCode_Success;
    }

    Log_Debug("Exiting with code %d.\n", exitCode);

    return exitCode;
}
