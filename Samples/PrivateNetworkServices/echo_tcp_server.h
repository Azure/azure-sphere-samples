/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "netinet/in.h"

#include "eventloop_timer_utilities.h"
#include "exitcode_privnetserv.h"

/// <summary>Reason why the TCP server stopped.</summary>
typedef enum {
    /// <summary>The echo server stopped because the client closed the connection.</summary>
    EchoServer_StopReason_ClientClosed,
    /// <summary>The echo server stopped because an error occurred.</summary>
    EchoServer_StopReason_Error
} EchoServer_StopReason;

/// <summary>
/// Bundles together state about an active echo server.
/// This should be allocated with <see cref="EchoServer_Start" /> and freed with
/// <see cref="EchoServer_ShutDown" />. The client should not directly modify member variables.
/// </summary>
typedef struct {
    /// <summary>Used to respond asynchronously to incoming connections.</summary>
    EventLoop *eventLoop;
    /// <summary>Socket which listens for incoming connections.</summary>
    int listenFd;
    /// <summary>Invoked when a new connection is received.</summary>
    EventRegistration *listenEventReg;
    /// <summary>Accept socket. Only one client socket is supported at a time.</summary>
    int clientFd;
    /// <summary>
    ///     Invoked when server receives data from or sends data to the client.
    /// </summary>
    EventRegistration *clientEventReg;
    /// <summary>Number of characters received from client.</summary>
    size_t inLineSize;
    /// <summary>Data received from client.</summary>
    char input[16];
    /// <summary>Payload to write to client.</summary>
    uint8_t *txPayload;
    /// <summary>Number of bytes to write to client.</summary>
    size_t txPayloadSize;
    /// <summary>Number of characters from paylod which have been written to client so
    /// far.</summary>
    size_t txBytesSent;
    /// <summary>
    ///     <para>Callback to invoke when the server stops processing connections.</para>
    ///     <para>
    ///         When this callback is invoked, the owner should clean up the server with
    ///         <see cref="EchoServer_ShutDown" />.
    ///     </para>
    ///     <param name="reason">Why the server stopped.</param>
    /// </summary>
    void (*shutdownCallback)(EchoServer_StopReason reason);
} EchoServer_ServerState;

/// <summary>
///     <para>Open a non-blocking TCP listening socket on the supplied IP address and port.</para>
///     <param name="eventLoopInstance">Event loop which will invoke IO callbacks.</param>
///     <param name="ipAddr">IP address to which the listen socket is bound.</param>
///     <param name="port">TCP port to which the socket is bound.</param>
///     <param name="backlogSize">Listening socket queue length.</param>
///     <param name="shutdownCallback">Callback to invoke when server shuts down.</param>
///     <param name="callerExitCode">
///         On failure, set to specific failure code. Undefined on success.
///     </param>
///     <returns>
///         Server state which is used to manage the server's resources, NULL on failure.
///         Should be disposed of with <see cref="EchoServer_ShutDown" />.
///     </returns>
/// </summary>
EchoServer_ServerState *EchoServer_Start(EventLoop *eventLoopInstance, in_addr_t ipAddr,
                                         uint16_t port, int backlogSize,
                                         void (*shutdownCallback)(EchoServer_StopReason),
                                         ExitCode *callerExitCode);

/// <summary>
/// <para>Closes any resources which were allocated by the supplied server. This includes
/// closing listen and accepted sockets, and freeing any heap memory that was allocated.</para>
/// <param name="serverState">Server state allocated with <see cref="EchoServer_Start" />.</param>
/// </summary>
void EchoServer_ShutDown(EchoServer_ServerState *serverState);
