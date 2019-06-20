/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application shows how to perform a DNS service discovery. It makes queries using
// multicast to local network and processes responses from the available DNS responders.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - networking (get network interface connection status)

#include "dns-sd.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

// File descriptors - initialized to invalid value
static int epollFd = -1;
static int timerFd = -1;
static int dnsSocketFd = -1;
static bool isNetworkStackReady = false;

// If using DNS in an internet-connected network, consider setting the desired status to be
// Networking_InterfaceConnectionStatus_ConnectedToInternet instead.
static const Networking_InterfaceConnectionStatus RequiredNetworkStatus =
    Networking_InterfaceConnectionStatus_IpAvailable;
static const char NetworkInterface[] = "wlan0";
static const char DnsServiceDiscoveryServer[] = "_sample-service._tcp.local";

// Termination state
static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Handle DNS service discover response received event.
/// </summary>
/// <param name="eventData">Context data for handled event.</param>
static void HandleReceivedDnsDiscoveryResponse(EventData *eventData)
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
        Log_Debug("INFO: DNS Service Discovery has found a instance: %s.\n", details->name);
        if (!details->host) {
            Log_Debug("INFO: Requesting SRV and TXT details for the instance.\n");
            SendServiceInstanceDetailsQuery(details->name, dnsSocketFd);
        } else {
            Log_Debug("\tName: %s\n\tHost: %s\n\tIPv4 Address: %s\n\tPort: %hd\n\tTXT Data: %.*s\n",
                      details->name, details->host, inet_ntoa(details->ipv4Address), details->port,
                      details->txtDataLength, details->txtData);
        }
    }

fail:
    FreeServiceInstanceDetails(details);
}

static EventData socketReceivedEventData = {.eventHandler = &HandleReceivedDnsDiscoveryResponse};

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
///     The timer event handler to check whether network connection is ready.
/// </summary>
static void ConnectionTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(timerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // Check whether the network connection is ready.
    bool isConnectionReady = false;
    if (IsConnectionReady(NetworkInterface, &isConnectionReady) != 0) {
        terminationRequired = true;
    } else if (isConnectionReady) {
        // Connection is ready, unregister the connection event handler. Register DNS response
        // handler, then start DNS service discovery.
        if (UnregisterEventHandlerFromEpoll(epollFd, timerFd) != 0 ||
            RegisterEventHandlerToEpoll(epollFd, dnsSocketFd, &socketReceivedEventData, EPOLLIN) !=
                0) {
            terminationRequired = true;
            return;
        }
        SendServiceDiscoveryQuery(DnsServiceDiscoveryServer, dnsSocketFd);
    }
}

static EventData timerEventData = {.eventHandler = &ConnectionTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure.</returns>
static int InitializeAndStartDnsServiceDiscovery(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        Log_Debug("ERROR: Failed to create epollFd.\n");
        return -1;
    }

    dnsSocketFd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if (dnsSocketFd < 0) {
        Log_Debug("ERROR: Failed to create dnsSocketFd: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Check network interface status at the specified period until it is ready.
    struct timespec checkInterval = {1, 0};
    timerFd = CreateTimerFdAndAddToEpoll(epollFd, &checkInterval, &timerEventData, EPOLLIN);
    if (timerFd < 0) {
        return -1;
    }
    return 0;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void Cleanup(void)
{
    Log_Debug("INFO: Closing file descriptors\n");
    CloseFdAndPrintError(epollFd, "Epoll");
    CloseFdAndPrintError(timerFd, "Timer");
    CloseFdAndPrintError(dnsSocketFd, "DNS Socket");
}

int main(void)
{
    Log_Debug("INFO: DNS Service Discovery sample starting.\n");
    if (InitializeAndStartDnsServiceDiscovery() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    Cleanup();
    Log_Debug("INFO: Application exiting.\n");
    return 0;
}