/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"

#include <applibs/log.h>
#include <applibs/networking.h>

#include "mt3620_rdb.h"

#include "echo_tcp_server.h"

// This sample C application listens for and responds to a TCP client. Both the listening and
// response are handled asynchronously using epoll events.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - networking (sets up private Ethernet configuration)

// File descriptors - initialized to invalid value
static int epollFd = -1;

EchoServer_ServerState *serverState = NULL;

// Termination state
static volatile sig_atomic_t terminationRequired = false;

// Ethernet / TCP server settings.
static const char LocalServerIp[] = "192.168.100.10";
static const uint16_t LocalTcpServerPort = 11000;
static const char SubnetMask[] = "255.255.255.0";
static const char GatewayIp[] = "0.0.0.0";
static int ServerBacklogSize = 3;
static const char NetworkInterface[] = "eth0";

// Support functions.
static void TerminationHandler(int signalNumber);

static int ConfigureNetworkAndLaunchServer(void);
static void ShutDownServer(void);
static int DisplayNetworkingInterfaces(void);
static int ConfigureNetworkInterface(in_addr_t ipAddr, in_addr_t subnetMask, in_addr_t gatewayAddr,
                                     const char *interfaceName);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
/// Called when the TCP server stops processing messages from clients.
/// </summary>
static void ServerStoppedHandler(EchoServer_StopReason reason)
{
    const char *reasonText;
    switch (reason) {
    case EchoServer_StopReason_ClientClosed:
        reasonText = "client closed the connection.";
        break;

    case EchoServer_StopReason_Error:
        reasonText = "an error occurred. See previous log output for more information.";
        break;

    default:
        reasonText = "unknown reason.";
        break;
    }

    Log_Debug("INFO: TCP server stopped: %s\n", reasonText);
    terminationRequired = true;
}

/// <summary>
///     Set up SIGTERM termination handler, set up epoll event handling, and start TCP server.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureNetworkAndLaunchServer(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    int result = DisplayNetworkingInterfaces();
    if (result != 0) {
        return -1;
    }

    struct in_addr ipAddr;
    inet_aton(LocalServerIp, &ipAddr);
    struct in_addr subnetMask;
    inet_aton(SubnetMask, &subnetMask);
    struct in_addr gatewayAddr;
    inet_aton(GatewayIp, &gatewayAddr);

    result = ConfigureNetworkInterface(ipAddr.s_addr, subnetMask.s_addr, gatewayAddr.s_addr,
                                       NetworkInterface);
    if (result != 0) {
        return -1;
    }

    serverState = EchoServer_Start(epollFd, ipAddr.s_addr, LocalTcpServerPort, ServerBacklogSize,
                                   ServerStoppedHandler);
    if (serverState == NULL) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Shut down TCP server and shut down epoll event handling.
/// </summary>
static void ShutDownServer(void)
{
    EchoServer_ShutDown(serverState);

    CloseFdAndPrintError(epollFd, "Epoll");
}

static int DisplayNetworkingInterfaces(void)
{
    // Display total number of network interfaces.
    ssize_t count = Networking_GetInterfaceCount();
    if (count == -1) {
        Log_Debug("ERROR: Networking_GetInterfaceCount: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: Networking_GetInterfaceCount: count=%zd\n", count);

    // Read current status of all interfaces.
    size_t bytesRequired = ((size_t)count) * sizeof(Networking_NetworkInterface);
    Networking_NetworkInterface *interfaces = malloc(bytesRequired);
    if (!interfaces) {
        abort();
    }

    ssize_t actualCount = Networking_GetInterfaces(interfaces, (size_t)count);
    if (actualCount == -1) {
        Log_Debug("ERROR: Networking_GetInterfaces: errno=%d (%s)\n", errno, strerror(errno));
    }
    Log_Debug("INFO: Networking_GetInterfaces: actualCount=%zd\n", actualCount);

    // Print detailed description of each interface.
    for (ssize_t i = 0; i < actualCount; ++i) {
        Log_Debug("INFO: interface #%zd\n", i);

        // Print the interface's name.
        char printName[IF_NAMESIZE + 1];
        memcpy(printName, interfaces[i].interfaceName, interfaces[i].interfaceNameLength);
        printName[interfaces[i].interfaceNameLength] = '\0';
        Log_Debug("INFO:   interfaceName=\"%s\"\n", interfaces[i].interfaceName);

        // Print whether the interface is enabled.
        Log_Debug("INFO:   isEnabled=\"%d\"\n", interfaces[i].isEnabled);

        // Print the interface's configuration type.
        Networking_IpConfiguration confType = interfaces[i].ipConfigurationType;
        const char *typeText;
        switch (confType) {
        case Networking_Networking_IpConfiguration_DhcpNone:
            typeText = "DhcpNone";
            break;
        case Networking_IpConfiguration_DhcpClient:
            typeText = "DhcpClient";
            break;
        default:
            typeText = "unknown-configuration-type";
            break;
        }
        Log_Debug("INFO:   ipConfigurationType=%d (%s)\n", confType, typeText);

        // Print the interface's medium.
        Networking_InterfaceMedium_Type mediumType = interfaces[i].interfaceMediumType;
        const char *mediumText;
        switch (mediumType) {
        case Networking_InterfaceMedium_Unspecified:
            mediumText = "unspecified";
            break;
        case Networking_InterfaceMedium_Wifi:
            mediumText = "Wi-Fi";
            break;
        case Networking_InterfaceMedium_Ethernet:
            mediumText = "Ethernet";
            break;
        default:
            mediumText = "unknown-medium";
            break;
        }
        Log_Debug("INFO:   interfaceMediumType=%d (%s)\n", mediumType, mediumText);
    }

    free(interfaces);

    return 0;
}

static int ConfigureNetworkInterface(in_addr_t ipAddr, in_addr_t subnetMask, in_addr_t gatewayAddr,
                                     const char *interfaceName)
{
    Networking_StaticIpConfiguration staticIpConfig;
    Networking_InitStaticIpConfiguration(&staticIpConfig);

    staticIpConfig.ipAddress.s_addr = ipAddr;
    staticIpConfig.netMask.s_addr = subnetMask;
    staticIpConfig.gatewayAddress.s_addr = gatewayAddr;

    int result = Networking_SetStaticIp(interfaceName, &staticIpConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_SetStaticIp: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("INFO: Private Ethernet TCP server application starting.\n");
    if (ConfigureNetworkAndLaunchServer() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ShutDownServer();
    Log_Debug("INFO: Application exiting.\n");
    return 0;
}
