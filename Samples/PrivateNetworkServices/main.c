/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application shows how to set up services on a private Ethernet network. It
// configures the network with a static IP address, starts the DHCP service allowing dynamically
// assigning IP address and network configuration parameters, enables the SNTP service allowing
// other devices to synchronize time via this device, and sets up a TCP server.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - networking (sets up private Ethernet configuration)

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
#include "eventloop_timer_utilities.h"

#include <applibs/log.h>
#include <applibs/networking.h>

// The following #include imports a "sample appliance" hardware definition. This provides a set of
// named constants such as SAMPLE_BUTTON_1 which are used when opening the peripherals, rather
// that using the underlying pin names. This enables the same code to target different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio. To target different hardware, you'll
// need to update the TARGET_HARDWARE variable in CMakeLists.txt - see instructions in that file.
//
// You can also use hardware definitions related to all other peripherals on your dev board because
// the sample_appliance header file recursively includes underlying hardware definition headers.
// See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
#include <hw/sample_appliance.h>

#include "echo_tcp_server.h"
#include "exitcode_privnetserv.h"

static void TerminationHandler(int signalNumber);
static void ServerStoppedHandler(EchoServer_StopReason reason);
static void ShutDownServerAndCleanup(void);
static ExitCode CheckNetworkStatus(void);
static ExitCode ConfigureNetworkInterfaceWithStaticIp(const char *interfaceName);
static ExitCode StartSntpServer(const char *interfaceName);
static ExitCode ConfigureAndStartDhcpSever(const char *interfaceName);
static ExitCode CheckNetworkStackStatusAndLaunchServers(void);
static void CheckStatusTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitializeAndLaunchServers(void);

static EventLoop *eventLoop = NULL;
static EventLoopTimer *checkStatusTimer = NULL;

static bool isNetworkStackReady = false;
EchoServer_ServerState *serverState = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

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
    exitCode = ExitCode_TermHandler_SigTerm;
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
    exitCode = ExitCode_StoppedHandler_Stopped;
}

/// <summary>
///     Shut down TCP server and close event handler.
/// </summary>
static void ShutDownServerAndCleanup(void)
{
    EchoServer_ShutDown(serverState);

    DisposeEventLoopTimer(checkStatusTimer);
    EventLoop_Close(eventLoop);
}

/// <summary>
///     Check network status and display information about all available network interfaces.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode CheckNetworkStatus(void)
{
    // Ensure the necessary network interface is enabled.
    int result = Networking_SetInterfaceState(NetworkInterface, true);
    if (result != 0) {
        if (errno == EAGAIN) {
            Log_Debug("INFO: The networking stack isn't ready yet, will try again later.\n");
            return ExitCode_Success;
        } else {
            Log_Debug(
                "ERROR: Networking_SetInterfaceState for interface '%s' failed: errno=%d (%s)\n",
                NetworkInterface, errno, strerror(errno));
            return ExitCode_CheckStatus_SetInterfaceState;
        }
    }
    isNetworkStackReady = true;

    // Display total number of network interfaces.
    ssize_t count = Networking_GetInterfaceCount();
    if (count == -1) {
        Log_Debug("ERROR: Networking_GetInterfaceCount: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_CheckStatus_GetInterfaceCount;
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
        Log_Debug("INFO:   interfaceName=\"%s\"\n", interfaces[i].interfaceName);

        // Print whether the interface is enabled.
        Log_Debug("INFO:   isEnabled=\"%d\"\n", interfaces[i].isEnabled);

        // Print the interface's configuration type.
        Networking_IpType ipType = interfaces[i].ipConfigurationType;
        const char *typeText;
        switch (ipType) {
        case Networking_IpType_DhcpNone:
            typeText = "DhcpNone";
            break;
        case Networking_IpType_DhcpClient:
            typeText = "DhcpClient";
            break;
        default:
            typeText = "unknown-configuration-type";
            break;
        }
        Log_Debug("INFO:   ipConfigurationType=%d (%s)\n", ipType, typeText);

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

        // Print the interface connection status
        Networking_InterfaceConnectionStatus status;
        int result = Networking_GetInterfaceConnectionStatus(interfaces[i].interfaceName, &status);
        if (result != 0) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: errno=%d (%s)\n", errno,
                      strerror(errno));
            return ExitCode_CheckStatus_GetInterfaceConnectionStatus;
        }
        Log_Debug("INFO:   interfaceStatus=0x%02x\n", status);
    }

    free(interfaces);

    return ExitCode_Success;
}

/// <summary>
///     Configure the specified network interface with a static IP address.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface to be configured.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ConfigureNetworkInterfaceWithStaticIp(const char *interfaceName)
{
    Networking_IpConfig ipConfig;
    Networking_IpConfig_Init(&ipConfig);
    inet_aton("192.168.100.10", &localServerIpAddress);
    inet_aton("255.255.255.0", &subnetMask);
    inet_aton("0.0.0.0", &gatewayIpAddress);
    Networking_IpConfig_EnableStaticIp(&ipConfig, localServerIpAddress, subnetMask,
                                       gatewayIpAddress);

    int result = Networking_IpConfig_Apply(interfaceName, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return ExitCode_ConfigureStaticIp_IpConfigApply;
    }
    Log_Debug("INFO: Set static IP address on network interface: %s.\n", interfaceName);

    return ExitCode_Success;
}

/// <summary>
///     Start SNTP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the SNTP server.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode StartSntpServer(const char *interfaceName)
{
    Networking_SntpServerConfig sntpServerConfig;
    Networking_SntpServerConfig_Init(&sntpServerConfig);
    int result = Networking_SntpServer_Start(interfaceName, &sntpServerConfig);
    Networking_SntpServerConfig_Destroy(&sntpServerConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_SntpServer_Start: %d (%s)\n", errno, strerror(errno));
        return ExitCode_StartSntpServer_StartSntp;
    }
    Log_Debug("INFO: SNTP server has started on network interface: %s.\n", interfaceName);
    return ExitCode_Success;
}

/// <summary>
///     Configure and start DHCP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the DHCP server.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ConfigureAndStartDhcpSever(const char *interfaceName)
{
    Networking_DhcpServerConfig dhcpServerConfig;
    Networking_DhcpServerConfig_Init(&dhcpServerConfig);

    struct in_addr dhcpStartIpAddress;
    inet_aton("192.168.100.11", &dhcpStartIpAddress);

    Networking_DhcpServerConfig_SetLease(&dhcpServerConfig, dhcpStartIpAddress, 1, subnetMask,
                                         gatewayIpAddress, 24);
    Networking_DhcpServerConfig_SetNtpServerAddresses(&dhcpServerConfig, &localServerIpAddress, 1);

    int result = Networking_DhcpServer_Start(interfaceName, &dhcpServerConfig);
    Networking_DhcpServerConfig_Destroy(&dhcpServerConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_DhcpServer_Start: %d (%s)\n", errno, strerror(errno));
        return ExitCode_StartDhcpServer_StartDhcp;
    }
    Log_Debug("INFO: DHCP server has started on network interface: %s.\n", interfaceName);
    return ExitCode_Success;
}

/// <summary>
///     Configure network interface, start SNTP server and TCP server.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode CheckNetworkStackStatusAndLaunchServers(void)
{
    // Check the network stack readiness and display available interfaces when it's ready.
    ExitCode localExitCode = CheckNetworkStatus();
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    // The network stack is ready, so unregister the timer event handler and launch servers.
    if (isNetworkStackReady) {
        DisarmEventLoopTimer(checkStatusTimer);

        // Use static IP addressing to configure network interface.
        localExitCode = ConfigureNetworkInterfaceWithStaticIp(NetworkInterface);
        if (localExitCode == ExitCode_Success) {
            localExitCode = StartSntpServer(NetworkInterface);
        }

        if (localExitCode == ExitCode_Success) {
            localExitCode = ConfigureAndStartDhcpSever(NetworkInterface);
        }

        if (localExitCode != ExitCode_Success) {
            return localExitCode;
        }

        // Start the TCP server.
        serverState = EchoServer_Start(eventLoop, localServerIpAddress.s_addr, LocalTcpServerPort,
                                       serverBacklogSize, ServerStoppedHandler, &localExitCode);
        if (serverState == NULL) {
            return localExitCode;
        }
    }

    return ExitCode_Success;
}

/// <summary>
///     The timer event handler.
/// </summary>
static void CheckStatusTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TimerHandler_Consume;
        return;
    }

    // Check whether the network stack is ready.
    if (!isNetworkStackReady) {
        ExitCode localExitCode = CheckNetworkStackStatusAndLaunchServers();
        if (localExitCode != ExitCode_Success) {
            exitCode = localExitCode;
            return;
        }
    }
}

/// <summary>
///     Set up SIGTERM termination handler, set up event loop, configure network
///     interface, start SNTP server and TCP server.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitializeAndLaunchServers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        return ExitCode_InitLaunch_EventLoop;
    }

    // Check network interface status at the specified period until it is ready.
    static const struct timespec checkInterval = {.tv_sec = 1, .tv_nsec = 0};
    checkStatusTimer =
        CreateEventLoopPeriodicTimer(eventLoop, CheckStatusTimerEventHandler, &checkInterval);
    if (checkStatusTimer == NULL) {
        return ExitCode_InitLaunch_Timer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("INFO: Private Ethernet TCP server application starting.\n");
    exitCode = InitializeAndLaunchServers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ShutDownServerAndCleanup();
    Log_Debug("INFO: Application exiting.\n");
    return exitCode;
}
