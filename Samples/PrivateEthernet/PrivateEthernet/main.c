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

// This sample C application shows how to set up services on a private Ethernet network. It
// configures the network with a static IP address, starts the DHCP service allowing dynamically
// assigning IP address and network configuration parameters, enables the SNTP service allowing
// other devices to synchronize time via this device, and sets up a TCP server.
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
static struct in_addr localServerIpAddress;
static struct in_addr subnetMask;
static struct in_addr gatewayIpAddress;
static const uint16_t LocalTcpServerPort = 11000;
static int serverBacklogSize = 3;
static const char NetworkInterface[] = "eth0";

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Called when the TCP server stops processing messages from clients.
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
///     Shut down TCP server and close epoll event handler.
/// </summary>
static void ShutDownServerAndCleanup(void)
{
    EchoServer_ShutDown(serverState);
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Display information about all available network interfaces.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
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

/// <summary>
///     Configure the specified network interface with a static IP address.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface to be configured.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureNetworkInterfaceWithStaticIp(const char *interfaceName)
{
    Networking_StaticIpConfiguration staticIpConfig;
    Networking_InitStaticIpConfiguration(&staticIpConfig);

    inet_aton("192.168.100.10", &localServerIpAddress);
    inet_aton("255.255.255.0", &subnetMask);
    inet_aton("0.0.0.0", &gatewayIpAddress);

    staticIpConfig.ipAddress.s_addr = localServerIpAddress.s_addr;
    staticIpConfig.netMask.s_addr = subnetMask.s_addr;
    staticIpConfig.gatewayAddress.s_addr = gatewayIpAddress.s_addr;

    int result = Networking_SetStaticIp(interfaceName, &staticIpConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_SetStaticIp: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

/// <summary>
///     Start SNTP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the SNTP server.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int StartSntpServer(const char *interfaceName)
{
    int result = Networking_StartSntpServer(interfaceName);
    if (result != 0) {
        Log_Debug("ERROR: Networking_StartSntpServer: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: SNTP server has started on network interface: %s.\n", interfaceName);
    return 0;
}

/// <summary>
///     Configure and start DHCP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the DHCP server.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureAndStartDhcpSever(const char *interfaceName)
{
    // Configure DHCP server to issue address 192.168.100.11 to a single client with 24-hour lease.
    Networking_DhcpServerConfiguration dhcpServerConfiguration;
    Networking_InitDhcpServerConfiguration(&dhcpServerConfiguration);
    struct in_addr dhcpStartIpAddress;
    inet_aton("192.168.100.11", &dhcpStartIpAddress);
    dhcpServerConfiguration.startIpAddress = dhcpStartIpAddress;
    dhcpServerConfiguration.ipAddressCount = 1;
    dhcpServerConfiguration.netMask = subnetMask;
    dhcpServerConfiguration.gatewayAddress = gatewayIpAddress;
    dhcpServerConfiguration.ntpServers[0] = localServerIpAddress;
    dhcpServerConfiguration.leaseTimeHours = 24;

    int result = Networking_StartDhcpServer(interfaceName, &dhcpServerConfiguration);
    if (result != 0) {
        Log_Debug("ERROR: Networking_StartDhcpServer: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: DHCP server has started on network interface: %s.\n", interfaceName);
    return 0;
}

/// <summary>
///     Set up SIGTERM termination handler, set up epoll event handling, configure network
///     interface, start SNTP server and start TCP server.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitializeAndLaunchServers(void)
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

    // Use static IP addressing to configure network interface.
    result = ConfigureNetworkInterfaceWithStaticIp(NetworkInterface);
    if (result != 0) {
        return -1;
    }

    // Configure and start DHCP server.
    result = ConfigureAndStartDhcpSever(NetworkInterface);
    if (result != 0) {
        return -1;
    }

    // Start the SNTP server.
    result = StartSntpServer(NetworkInterface);
    if (result != 0) {
        return -1;
    }

    // Start the TCP server.
    serverState = EchoServer_Start(epollFd, localServerIpAddress.s_addr, LocalTcpServerPort,
                                   serverBacklogSize, ServerStoppedHandler);
    if (serverState == NULL) {
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
    if (InitializeAndLaunchServers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ShutDownServerAndCleanup();
    Log_Debug("INFO: Application exiting.\n");
    return 0;
}
