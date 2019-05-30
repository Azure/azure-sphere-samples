/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere sends messages to, and receives
// responses from, the real-time core.  It sends a message every second and prints
// the message which was sent, and the response which was received.
//
// It uses the following Azure Sphere libraries
// - log (messages shown in Visual Studio's Device Output window during debugging);
// - application (establish a connection with a real-time capable application).

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/socket.h>

#include <applibs/log.h>
#include <applibs/application.h>

#include "epoll_timerfd_utilities.h"

static int epollFd = -1;
static int timerFd = -1;
static int sockFd = -1;
static volatile sig_atomic_t terminationRequired = false;

static const char rtAppComponentId[] = "005180bc-402f-4cb3-a662-72937dbcde47";

static void TerminationHandler(int signalNumber);
static void TimerEventHandler(EventData *eventData);
static void SendMessageToRTCore(void);
static void SocketEventHandler(EventData *eventData);
static int InitHandlers(void);
static void CloseHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Handle send timer event by writing data to the real-time capable application.
/// </summary>
static void TimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(timerFd) != 0) {
        terminationRequired = true;
        return;
    }

    SendMessageToRTCore();
}

/// <summary>
///     Helper function for TimerEventHandler sends message to real-time capable application.
/// </summary>
static void SendMessageToRTCore(void)
{
    static int iter = 0;

    // Send "HELLO-WORLD-%d" message to real-time capable application.
    static char txMessage[32];
    sprintf(txMessage, "Hello-World-%d", iter++);
    Log_Debug("Sending: %s\n", txMessage);

    int bytesSent = send(sockFd, txMessage, strlen(txMessage), 0);
    if (bytesSent == -1) {
        Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
        terminationRequired = true;
        return;
    }
}

/// <summary>
///     Handle socket event by reading incoming data from real-time capable application.
/// </summary>
static void SocketEventHandler(EventData *eventData)
{
    // Read response from real-time capable application.
    char rxBuf[32];
    int bytesReceived = recv(sockFd, rxBuf, sizeof(rxBuf), 0);

    if (bytesReceived == -1) {
        Log_Debug("ERROR: Unable to receive message: %d (%s)\n", errno, strerror(errno));
        terminationRequired = true;
    }

    Log_Debug("Received %d bytes: ", bytesReceived);
    for (int i = 0; i < bytesReceived; ++i) {
        Log_Debug("%c", isprint(rxBuf[i]) ? rxBuf[i] : '.');
    }
    Log_Debug("\n");
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData timerEventData = {.eventHandler = &TimerEventHandler};
static EventData socketEventData = {.eventHandler = &SocketEventHandler};

/// <summary>
///     Set up SIGTERM termination handler and event handlers for send timer
///     and to receive data from real-time capable application.
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

    // Register one second timer to send a message to the real-time core.
    static const struct timespec sendPeriod = {.tv_sec = 1, .tv_nsec = 0};
    timerFd = CreateTimerFdAndAddToEpoll(epollFd, &sendPeriod, &timerEventData, EPOLLIN);
    if (timerFd < 0) {
        return -1;
    }
    RegisterEventHandlerToEpoll(epollFd, timerFd, &timerEventData, EPOLLIN);

    // Open connection to real-time capable application.
    sockFd = Application_Socket(rtAppComponentId);
    if (sockFd == -1) {
        Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Set timeout, to handle case where real-time capable application does not respond.
    static const struct timeval recvTimeout = {.tv_sec = 5, .tv_usec = 0};
    int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
    if (result == -1) {
        Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Register handler for incoming messages from real-time capable application.
    if (RegisterEventHandlerToEpoll(epollFd, sockFd, &socketEventData, EPOLLIN) != 0) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(sockFd, "Socket");
    CloseFdAndPrintError(timerFd, "Timer");
    CloseFdAndPrintError(epollFd, "Epoll");
}

int main(void)
{
    Log_Debug("High-level intercore application.\n");
    Log_Debug("Sends data to, and receives data from the real-time core.\n");

    if (InitHandlers() != 0) {
        terminationRequired = true;
    }

    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    CloseHandlers();
    Log_Debug("Application exiting.\n");
    return 0;
}
