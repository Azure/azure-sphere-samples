/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#define _GNU_SOURCE // required for asprintf
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>

#include <sys/socket.h>

#include <applibs/log.h>

#include "echo_tcp_server.h"

// Support functions.
static void HandleListenEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void LaunchRead(EchoServer_ServerState *serverState);
static void HandleClientEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void HandleClientReadEvent(EchoServer_ServerState *serverState);
static void LaunchWrite(EchoServer_ServerState *serverState);
static void HandleClientWriteEvent(EchoServer_ServerState *serverState);
static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType, ExitCode *callerExitCode);
static void ReportError(const char *desc);
static void StopServer(EchoServer_ServerState *serverState, EchoServer_StopReason reason);

EchoServer_ServerState *EchoServer_Start(EventLoop *eventLoopInstance, in_addr_t ipAddr,
                                         uint16_t port, int backlogSize,
                                         void (*shutdownCallback)(EchoServer_StopReason),
                                         ExitCode *callerExitCode)
{
    EchoServer_ServerState *serverState = malloc(sizeof(*serverState));
    if (!serverState) {
        abort();
    }

    // Set EchoServer_ServerState state to unused values so it can be safely cleaned up if only a
    // subset of the resources are successfully allocated.
    serverState->eventLoop = eventLoopInstance;
    serverState->listenFd = -1;
    serverState->listenEventReg = NULL;
    serverState->clientFd = -1;
    serverState->clientEventReg = NULL;
    serverState->txPayload = NULL;
    serverState->shutdownCallback = shutdownCallback;

    int sockType = SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK;
    serverState->listenFd = OpenIpV4Socket(ipAddr, port, sockType, callerExitCode);
    if (serverState->listenFd == -1) {
        ReportError("open socket");
        goto fail;
    }

    // Be notified asynchronously when a client connects.
    serverState->listenEventReg = EventLoop_RegisterIo(
        eventLoopInstance, serverState->listenFd, EventLoop_Input, HandleListenEvent, serverState);
    if (serverState->listenEventReg == NULL) {
        ReportError("register listen event");
        goto fail;
    }

    int result = listen(serverState->listenFd, backlogSize);
    if (result != 0) {
        ReportError("listen");
        *callerExitCode = ExitCode_EchoStart_Listen;
        goto fail;
    }

    Log_Debug("INFO: TCP server: Listening for client connection (fd %d).\n",
              serverState->listenFd);

    return serverState;

fail:
    EchoServer_ShutDown(serverState);
    return NULL;
}

static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

void EchoServer_ShutDown(EchoServer_ServerState *serverState)
{
    if (!serverState) {
        return;
    }

    EventLoop_UnregisterIo(serverState->eventLoop, serverState->clientEventReg);
    CloseFdAndPrintError(serverState->clientFd, "clientFd");

    EventLoop_UnregisterIo(serverState->eventLoop, serverState->listenEventReg);
    CloseFdAndPrintError(serverState->listenFd, "listenFd");

    free(serverState->txPayload);

    free(serverState);
}

static void HandleListenEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    EchoServer_ServerState *serverState = (EchoServer_ServerState *)context;
    int localFd = -1;

    do {
        // Create a new accepted socket to connect to the client.
        // The newly-accepted sockets should be opened in non-blocking mode.
        struct sockaddr in_addr;
        socklen_t sockLen = sizeof(in_addr);
        localFd = accept4(serverState->listenFd, &in_addr, &sockLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (localFd == -1) {
            ReportError("accept");
            break;
        }

        Log_Debug("INFO: TCP server: Accepted client connection (fd %d).\n", localFd);

        // If already have a client, then close the newly-accepted socket.
        if (serverState->clientFd >= 0) {
            Log_Debug(
                "INFO: TCP server: Closing incoming client connection: only one client supported "
                "at a time.\n");
            break;
        }

        serverState->clientEventReg = EventLoop_RegisterIo(serverState->eventLoop, localFd, 0x0,
                                                           HandleClientEvent, serverState);
        if (serverState->clientEventReg == NULL) {
            ReportError("register client event");
            break;
        }

        // Socket opened successfully, so transfer ownership to EchoServer_ServerState object.
        serverState->clientFd = localFd;
        localFd = -1;

        LaunchRead(serverState);
    } while (0);

    CloseFdAndPrintError(localFd, "localClientFd");
}

static void LaunchRead(EchoServer_ServerState *serverState)
{
    serverState->inLineSize = 0;

    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_Input);
}

static void HandleClientEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    EchoServer_ServerState *serverState = context;

    if (events & EventLoop_Input) {
        HandleClientReadEvent(serverState);
    }

    if (events & EventLoop_Output) {
        HandleClientWriteEvent(serverState);
    }
}

static void HandleClientReadEvent(EchoServer_ServerState *serverState)
{
    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_None);

    // Continue until no immediately available input or until an error occurs.
    size_t maxChars = sizeof(serverState->input) - 1;

    while (true) {
        // Read a single byte from the client and add it to the buffered line.
        uint8_t b;
        ssize_t bytesReadOneSysCall = recv(serverState->clientFd, &b, 1, /* flags */ 0);

        // If successfully read a single byte then process it.
        if (bytesReadOneSysCall == 1) {
            // If received newline then print received line to debug log.
            if (b == '\r') {
                serverState->input[serverState->inLineSize] = '\0';
                Log_Debug("INFO: TCP server: Received \"%s\"\n", serverState->input);
                LaunchWrite(serverState);
                break;
            }

            // If new character is not printable then discard.
            else if (!isprint(b)) {
                // Special case '\n' to avoid printing a message for every line of input.
                if (b != '\n') {
                    Log_Debug("INFO: TCP server: Discarding unprintable character 0x%02x\n", b);
                }
            }

            // If new character would leave no space for NUL terminator then reset buffer.
            else if (serverState->inLineSize == maxChars) {
                Log_Debug("INFO: TCP server: Input data overflow. Discarding %zu characters.\n",
                          maxChars);
                serverState->input[0] = b;
                serverState->inLineSize = 1;
            }

            // Else append character to buffer.
            else {
                serverState->input[serverState->inLineSize] = b;
                ++serverState->inLineSize;
            }
        }

        // If client has shut down cleanly then terminate.
        else if (bytesReadOneSysCall == 0) {
            Log_Debug("INFO: TCP server: Client has closed connection, so terminating server.\n");
            StopServer(serverState, EchoServer_StopReason_ClientClosed);
            break;
        }

        // If receive buffer is empty then wait for EventLoop_Input event.
        else if (bytesReadOneSysCall == -1 && errno == EAGAIN) {
            EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg,
                                     EventLoop_Input);
            break;
        }

        // Another error occured so abort the program.
        else {
            ReportError("recv");
            StopServer(serverState, EchoServer_StopReason_Error);
            break;
        }
    }
}

static void LaunchWrite(EchoServer_ServerState *serverState)
{
    // Allocate a client response on the heap.
    char *str;
    int result = asprintf(&str, "Received \"%s\"\r\n", serverState->input);
    if (result == -1) {
        ReportError("asprintf");
        StopServer(serverState, EchoServer_StopReason_Error);
        return;
    }

    // Start to send the response.
    serverState->txPayloadSize = (size_t)result;
    serverState->txPayload = (uint8_t *)str;
    serverState->txBytesSent = 0;
    HandleClientWriteEvent(serverState);
}

/// <summary>
///     <para>
///         Called to launch a new write operation, or to continue an existing
///         write operation when the client socket receives a write event.
///     </para>
///     <param name="serverState">
///         The server whose client should be sent the message.
///     </param>
/// </summary>
static void HandleClientWriteEvent(EchoServer_ServerState *serverState)
{
    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_None);

    // Continue until have written entire response, error occurs, or OS TX buffer is full.
    while (serverState->txBytesSent < serverState->txPayloadSize) {
        size_t remainingBytes = serverState->txPayloadSize - serverState->txBytesSent;
        const uint8_t *data = &serverState->txPayload[serverState->txBytesSent];
        ssize_t bytesSentOneSysCall =
            send(serverState->clientFd, data, remainingBytes, /* flags */ 0);

        // If successfully sent data then stay in loop and try to send more data.
        if (bytesSentOneSysCall > 0) {
            serverState->txBytesSent += (size_t)bytesSentOneSysCall;
        }

        // If OS TX buffer is full then wait for next EventLoop_Output.
        else if (bytesSentOneSysCall < 0 && errno == EAGAIN) {
            EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg,
                                     EventLoop_Output);
            return;
        }

        // Another error occurred so terminate the program.
        else {
            ReportError("send");
            StopServer(serverState, EchoServer_StopReason_Error);
            return;
        }
    }

    // If reached here then successfully sent entire payload so clean up and read next line from
    // client.
    free(serverState->txPayload);
    serverState->txPayload = NULL;

    LaunchRead(serverState);
}

static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType, ExitCode *callerExitCode)
{
    int localFd = -1;
    int retFd = -1;

    do {
        // Create a TCP / IPv4 socket. This will form the listen socket.
        localFd = socket(AF_INET, sockType, /* protocol */ 0);
        if (localFd == -1) {
            ReportError("socket");
            *callerExitCode = ExitCode_OpenIpV4_Socket;
            break;
        }

        // Enable rebinding soon after a socket has been closed.
        int enableReuseAddr = 1;
        int r = setsockopt(localFd, SOL_SOCKET, SO_REUSEADDR, &enableReuseAddr,
                           sizeof(enableReuseAddr));
        if (r != 0) {
            ReportError("setsockopt/SO_REUSEADDR");
            *callerExitCode = ExitCode_OpenIpV4_SetSockOpt;
            break;
        }

        // Bind to a well-known IP address.
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ipAddr;
        addr.sin_port = htons(port);

        r = bind(localFd, (const struct sockaddr *)&addr, sizeof(addr));
        if (r != 0) {
            ReportError("bind");
            *callerExitCode = ExitCode_OpenIpV4_Bind;
            break;
        }

        // Port opened successfully.
        retFd = localFd;
        localFd = -1;
    } while (0);

    CloseFdAndPrintError(localFd, "localListenFd");

    return retFd;
}

static void ReportError(const char *desc)
{
    Log_Debug("ERROR: TCP server: \"%s\", errno=%d (%s)\n", desc, errno, strerror(errno));
}

static void StopServer(EchoServer_ServerState *serverState, EchoServer_StopReason reason)
{
    if (serverState->clientEventReg != NULL) {
        EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg,
                                 EventLoop_None);
    }

    if (serverState->listenEventReg != NULL) {
        EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->listenEventReg,
                                 EventLoop_None);
    }

    serverState->shutdownCallback(reason);
}
