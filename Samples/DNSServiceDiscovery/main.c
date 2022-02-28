/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application shows how to perform a DNS service discovery. It makes queries using
// multicast to local network and processes responses from the available DNS responders.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - networking (get network interface connection status)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

#include <applibs/log.h>
#include <applibs/networking.h>

#include "dns-sd.h"
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ConnectionTimer_Consume = 2,
    ExitCode_ConnectionTimer_ConnectionReady = 3,
    ExitCode_ConnectionTimer_Disarm = 4,

    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_Socket = 6,

    ExitCode_Init_ConnectionTimer = 7,
    ExitCode_Init_DnsResponseHandler = 8,

    ExitCode_Main_EventLoopFail = 9
} ExitCode;

// File descriptors - initialized to invalid value
static int dnsSocketFd = -1;
static bool isNetworkStackReady = false;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *connectionTimer = NULL;
static EventRegistration *dnsEventReg = NULL;

// If using DNS in an internet-connected network, consider setting the desired status to be
// Networking_InterfaceConnectionStatus_ConnectedToInternet instead.
static const Networking_InterfaceConnectionStatus RequiredNetworkStatus =
    Networking_InterfaceConnectionStatus_IpAvailable;
static const char NetworkInterface[] = "wlan0";
static const char DnsServiceDiscoveryServer[] = "_sample-service._tcp.local";

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void HandleReceivedDnsDiscoveryResponse(EventLoop *el, int fd, EventLoop_IoEvents events,
                                               void *context);
static void ConnectionTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitializeAndStartDnsServiceDiscovery(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void Cleanup(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Handle DNS service discover response received event.
///     This function follows the <see cref="EventLoopIoCallback" /> signature.
/// </summary>
/// <param name="el"> The EventLoop in which the callback was registered </param>
/// <param name="fd"> The descriptor for which the event fired </param>
/// <param name="events"> The bitmask of events fired </param>
/// <param name="context"> An optional context pointer that was passed in the registration. </param>
static void HandleReceivedDnsDiscoveryResponse(EventLoop *el, int fd, EventLoop_IoEvents events,
                                               void *context)
{
    // Read received DNS response over socket.
    // Process received response, if it contains PRT message, but no SRV or TXT message, send
    // instance details request for each PTR instance. Otherwise, get the instance details.
    ServiceInstanceDetails *details = NULL;
    int result = ProcessDnsResponse(dnsSocketFd, &details);
    if (result != 0) {
        goto fail;
    }

    if (details && details->name) {
        Log_Debug("INFO: DNS Service Discovery has found an instance: %s.\n", details->name);
        if (!details->host) {
            Log_Debug("INFO: Requesting SRV and TXT details for the instance.\n");
            SendServiceInstanceDetailsQuery(details->name, dnsSocketFd);
        } else {
            // NOTE: The TXT data is simply treated as a string and isn't parsed here. You should
            // replace this with your own production logic.
            Log_Debug("\tName: %s\n\tHost: %s\n\tIPv4 Address: %s\n\tPort: %hd\n\tTXT Data: %.*s\n",
                      details->name, details->host, inet_ntoa(details->ipv4Address), details->port,
                      details->txtDataLength, details->txtData);
        }
    }

fail:
    FreeServiceInstanceDetails(details);
}

/// <summary>
///     Check whether the required network connection status has been met.
/// </summary>
/// <param name="interface">The network interface to perform the check on.</param>
/// <param name="ipAddressAvailable">The result of the connection status check.</param>
/// <returns>0 on success, or -1 on failure.</returns>
int IsConnectionReady(const char *interface, bool *ipAddressAvailable)
{
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(interface, &status) == 0) {
        Log_Debug("INFO: Network interface %s status: 0x%02x\n", interface, status);
        isNetworkStackReady = true;
    } else {
        if (errno == EAGAIN) {
            Log_Debug("INFO: The networking stack isn't ready yet, will try again later.\n");
            return 0;
        } else {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                      strerror(errno));
            return -1;
        }
    }
    *ipAddressAvailable = (status & RequiredNetworkStatus) != 0;
    return 0;
}

/// <summary>
///     The timer event handler to check whether network connection is ready and send DNS service
///     discovery queries.
/// </summary>
static void ConnectionTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ConnectionTimer_Consume;
        return;
    }

    // Check whether the network connection is ready.
    bool isConnectionReady = false;
    if (IsConnectionReady(NetworkInterface, &isConnectionReady) != 0) {
        exitCode = ExitCode_ConnectionTimer_ConnectionReady;
    } else if (isConnectionReady) {
        // Connection is ready, send a DNS service discovery query.
        SendServiceDiscoveryQuery(DnsServiceDiscoveryServer, dnsSocketFd);
    }
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitializeAndStartDnsServiceDiscovery(void)
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

    dnsSocketFd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if (dnsSocketFd == -1) {
        Log_Debug("ERROR: Failed to create dnsSocketFd: %d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_Socket;
    }

    // Check network interface status at the specified period until it is ready.
    // This also defines the frequency at which the sample sends out DNS-SD queries.
    static const struct timespec checkInterval = {.tv_sec = 10, .tv_nsec = 0};
    connectionTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ConnectionTimerEventHandler, &checkInterval);
    if (connectionTimer == NULL) {
        return ExitCode_Init_ConnectionTimer;
    }

    // Register DNS response handler, for handling responses from DNS-SD queries.
    dnsEventReg = EventLoop_RegisterIo(eventLoop, dnsSocketFd, EventLoop_Input,
                                       &HandleReceivedDnsDiscoveryResponse, /* context */ NULL);
    if (dnsEventReg == NULL) {
        return ExitCode_Init_DnsResponseHandler;
    }

    return ExitCode_Success;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void Cleanup(void)
{
    DisposeEventLoopTimer(connectionTimer);
    EventLoop_UnregisterIo(eventLoop, dnsEventReg);
    EventLoop_Close(eventLoop);

    Log_Debug("INFO: Closing file descriptors\n");
    CloseFdAndPrintError(dnsSocketFd, "DNS Socket");
}

int main(void)
{
    Log_Debug("INFO: DNS Service Discovery sample starting.\n");

    exitCode = InitializeAndStartDnsServiceDiscovery();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    Cleanup();
    Log_Debug("INFO: Application exiting.\n");
    return exitCode;
}