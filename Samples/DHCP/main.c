/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the DHCP client APIs.
// It shows how to:
// 1. Renew the current IP address.
// 2. Release the current IP address.

// It uses the API for the following Azure Sphere application libraries:
// - eventloop (system invokes handlers for timer events).
// - gpio (digital input for button, digital output for LED).
// - log (displays messages in the Device Output window during debugging).
// - networking (functions to renew/release the current IP address from network's DHCP server).

// You will need to configure the network interface to be used in the constant variable below.
// Please see README.md for full details.

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>

#include <applibs/eventloop.h>
#include <applibs/gpio.h>
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

#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_IsButtonPressed_GetValue = 2,
    ExitCode_ButtonTimer_Consume = 3,
    ExitCode_SyncStatusTimer_Consume = 4,
    ExitCode_Init_EventLoop = 5,

    ExitCode_Init_Button1Open = 6,
    ExitCode_Init_Button2Open = 7,
    ExitCode_Init_ButtonPollTimer = 8,

    ExitCode_Init_RedLed = 9,
    ExitCode_Init_BlueLed = 10,
    ExitCode_Init_GreenLed = 11,

    ExitCode_Init_CreateIpSyncStatusTimer = 1,

    ExitCode_IpConfig_ReleaseIp_Failed = 12,
    ExitCode_IpConfig_RenewIp_Failed = 13,

    ExitCode_Main_SetEnv = 22,
    ExitCode_Main_EventLoopFail = 23
} ExitCode;

/// <summary>
///     The available network interface device names.
/// </summary>
#define NET_INTERFACE_WLAN "wlan0"
#define NET_INTERFACE_ETHERNET "eth0"

// User configuration.
const char *const currentNetInterface = NET_INTERFACE_WLAN;

// File descriptors - initialized to invalid value.
static int releaseIpButtonGpioFd = -1;
static int renewIpButtonGpioFd = -1;

// The LEDs show the current network interface's status:
// - Interface unavailable: all LEDs off
// - InterfaceUp: (SAMPLE_RGBLED_RED)
// - ConnectedToNetwork: (SAMPLE_RGBLED_GREEN) + (SAMPLE_RGBLED_RED) ==> YELLOW
// - IpAvailable: (SAMPLE_RGBLED_BLUE)
// - ConnectedToInternet: (SAMPLE_RGBLED_GREEN)
static int ipUnassignedLedRedGpioFd = -1;
static int ipAssignedLedBlueGpioFd = -1;
static int inetAvailableLedGreenGpioFd = -1;

// Button state variables.
static GPIO_Value_Type getLastIpReleaseInfoButtonState = GPIO_Value_High;
static GPIO_Value_Type getLastIpRenewInfoButtonState = GPIO_Value_High;

// The event loop handling both button presses and network interface states.
static EventLoop *eventLoop = NULL;

// The button presses are checked every 50 milliseconds.
static EventLoopTimer *buttonPollTimer = NULL;
static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 50 * 1000 * 1000};

// The IP sync status is checked every second.
static EventLoopTimer *networkStatusPollTimer = NULL;
static const struct timespec networkStatusCheckPeriod = {.tv_sec = 1, .tv_nsec = 0};

// Termination state.
static volatile sig_atomic_t exitCode = ExitCode_Success;

// Function prototypes
static void TerminationHandler(int signalNumber);
static void SetLedStates(bool red, bool blue, bool green);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static void EnableCurrentNetworkInterface(void);
static void NetworkConnectionStatusTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);
static void ReleaseIpConfig(void);
static void RenewIpConfig(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Utility for setting all the LED states at once.
/// </summary>
/// <param name="red"> Turns ON (true) or OFF (false) the red LED.</param>
/// <param name="green"> Turns ON (true) or OFF (false) the green LED.</param>
/// <param name="blue"> Turns ON (true) or OFF (false) the blue LED.</param>
static void SetLedStates(bool red, bool green, bool blue)
{
    if (ipUnassignedLedRedGpioFd != -1) {
        GPIO_SetValue(ipUnassignedLedRedGpioFd, red ? GPIO_Value_Low : GPIO_Value_High);
    }
    if (inetAvailableLedGreenGpioFd != -1) {
        GPIO_SetValue(inetAvailableLedGreenGpioFd, green ? GPIO_Value_Low : GPIO_Value_High);
    }
    if (ipAssignedLedBlueGpioFd != -1) {
        GPIO_SetValue(ipAssignedLedBlueGpioFd, blue ? GPIO_Value_Low : GPIO_Value_High);
    }
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="buttonState">Old state of the button (pressed or released)
/// which will be updated to the current button state.
/// </param>
/// <returns>True if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *buttonState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;

    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: could not read button GPIO: errno=%d (%s).\n", errno, strerror(errno));
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *buttonState) && (newState == GPIO_Value_Low);
        *buttonState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Button timer event: check the status of the buttons.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check if BUTTON_1 (A) was pressed.
    if (IsButtonPressed(releaseIpButtonGpioFd, &getLastIpReleaseInfoButtonState)) {
        ReleaseIpConfig();
    }

    // Check if BUTTON_2 (B) was pressed.
    if (IsButtonPressed(renewIpButtonGpioFd, &getLastIpRenewInfoButtonState)) {
        RenewIpConfig();
    }
}

/// <summary>
///     Attempts to retrieve the current network interface's IP address.
/// </summary>
/// <param name=""></param>
char *GetIpAddress(void)
{
    static char ip_address[sizeof("000.000.000.000")];

    // Find the assigned IP address by scanning all the interfaces.
    struct ifaddrs *addr_list;
    *ip_address = 0;

    if (getifaddrs(&addr_list) != 0) {
        Log_Debug("ERROR: getifaddrs() failed: errno=%d (%s)\n", errno, strerror(errno));

    } else {

        struct ifaddrs *it = addr_list;
        for (int n = 0; it != NULL; it = it->ifa_next, ++n) {
            if (NULL == it->ifa_addr)
                continue;

            if (0 == strncmp(it->ifa_name, currentNetInterface, strlen(currentNetInterface))) {
                if (AF_INET == it->ifa_addr->sa_family) {
                    struct sockaddr_in *addr = (struct sockaddr_in *)it->ifa_addr;
                    strncpy(ip_address, inet_ntoa(addr->sin_addr), sizeof(ip_address) - 1);
                }
            }
        }
    }
    freeifaddrs(addr_list);

    return ip_address;
}

/// <summary>
///     Attempts to enable the current network interface, specified in the `currentNetInterface`
///     global variable.
/// </summary>
/// <param name=""></param>
static void EnableCurrentNetworkInterface(void)
{
    Log_Debug("INFO: Attempting to enable network interface '%s'.\n", currentNetInterface);
    int res = Networking_SetInterfaceState(currentNetInterface, true);
    if (-1 == res) {
        Log_Debug("ERROR: enabling network interface '%s': errno= %d (%s).\n", currentNetInterface,
                  errno, strerror(errno));
    } else {
        Log_Debug("INFO: Network interface is now set to '%s'.\n", currentNetInterface);

        // If the network is on Wi-Fi, then disable the Ethernet interface (and vice versa).
        bool onWiFi = (0 == strcmp(currentNetInterface, NET_INTERFACE_WLAN));
        Networking_SetInterfaceState(onWiFi ? NET_INTERFACE_ETHERNET : NET_INTERFACE_WLAN, false);
        if (-1 == res) {
            Log_Debug("ERROR: Disabling network interface '%s': errno=%d (%s).\n",
                      onWiFi ? NET_INTERFACE_ETHERNET : NET_INTERFACE_WLAN, errno, strerror(errno));
        }
    }
}

/// <summary>
///     Network connection status timer: checks if the current DHCP has assigned
///     an IP address.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
static void NetworkConnectionStatusTimerEventHandler(EventLoopTimer *timer)
{
    static Networking_InterfaceConnectionStatus interfaceStatus = 0;

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_SyncStatusTimer_Consume;
        return;
    }

    // For the UX purposes of this sample, the Networking_GetInterfaceConnectionStatus()
    // API is called more frequently than the minimum recommended interval, which may lead
    // to receive transient states in return. These are managed below in the switch's default
    // statement.
    Networking_InterfaceConnectionStatus currentInterfaceStatus;
    if (-1 ==
        Networking_GetInterfaceConnectionStatus(currentNetInterface, &currentInterfaceStatus)) {
        Log_Debug("ERROR: retrieving the '%s' network interface's status: errno=%d (%s).\n",
                  currentNetInterface, errno, strerror(errno));
    }

    // Keep logging and displaying the current network interface's state changes, or
    // in case the network interface is unavailable, we attempt enabling it.
    if (interfaceStatus == 0 || interfaceStatus != currentInterfaceStatus) {

        interfaceStatus = currentInterfaceStatus;

        switch (interfaceStatus) {

        case 0:
            // The network interface is unavailable.
            // Turn all LEDs off.
            SetLedStates(false, false, false);
            Log_Debug("ERROR: network interface '%s' NOT ready!\n", currentNetInterface);
            EnableCurrentNetworkInterface();
            break;

        case Networking_InterfaceConnectionStatus_InterfaceUp:
            // The network interface is up and available, but hasn't yet connected
            // to the network.
            // Turn on the RED LED.
            SetLedStates(true, false, false);
            Log_Debug("INFO: Network interface '%s' is up but not connected to the network.\n",
                      currentNetInterface);
            break;

        case (Networking_InterfaceConnectionStatus_InterfaceUp |
              Networking_InterfaceConnectionStatus_ConnectedToNetwork):
            // The network interface is up and connected to the network, but hasn't yet
            // received an IP address from the network's DHCP server.
            // Turn on the RED+GREEN LEDs for a YELLOW.
            SetLedStates(true, true, false);
            Log_Debug(
                "INFO: Network interface '%s' is connected to the network (no IP address "
                "assigned).\n",
                currentNetInterface);
            break;

        case (Networking_InterfaceConnectionStatus_InterfaceUp |
              Networking_InterfaceConnectionStatus_ConnectedToNetwork |
              Networking_InterfaceConnectionStatus_IpAvailable):
            // The network interface is up, connected to the network and successfully
            // acquired an IP address from the network's DHCP server.
            // Turn on the BLUE LED.
            SetLedStates(false, false, true);
            Log_Debug(
                "INFO: Network interface '%s' is connected and has been assigned "
                "IP address [%s].\n",
                currentNetInterface, GetIpAddress());
            break;

        case (Networking_InterfaceConnectionStatus_InterfaceUp |
              Networking_InterfaceConnectionStatus_ConnectedToNetwork |
              Networking_InterfaceConnectionStatus_IpAvailable |
              Networking_InterfaceConnectionStatus_ConnectedToInternet):
            // The network interface is fully operative and connected up to the Internet.
            // Turn on the GREEN LED.
            SetLedStates(false, true, false);
            Log_Debug(
                "INFO: Network interface '%s' is connected to the Internet "
                "(local IP address [%s]).\n",
                currentNetInterface, GetIpAddress());
            break;

        default:
            // The network interface is in a transient state.
            // Turn all LEDs off.
            SetLedStates(false, false, false);
            Log_Debug("INFO: Network interface '%s' is in a transient state [0x%04x].\n",
                      currentNetInterface, interfaceStatus);
            break;
        }
    }
}

/// <summary>
///     Requests a DHCP Release for the current IP address.
/// </summary>
static void ReleaseIpConfig(void)
{
    int iRes = Networking_IpConfig_ReleaseIp(currentNetInterface);
    if (iRes == -1) {
        Log_Debug("ERROR: Networking_IpConfig_ReleaseIp() failed: errno=%d (%s)\n", errno,
                  strerror(errno));
        return;
    }

    Log_Debug("INFO: Successfully released the IP address.\n");
}

/// <summary>
///     Requests a DHCP Renew for the current IP address.
/// </summary>
static void RenewIpConfig(void)
{
    int iRes = Networking_IpConfig_RenewIp(currentNetInterface);
    if (iRes == -1) {
        Log_Debug("ERROR: Networking_IpConfig_RenewIp() failed: errno=%d (%s)\n", errno,
                  strerror(errno));
        return;
    }

    Log_Debug("INFO: Successfully renewed the IP address.\n");
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
            Log_Debug("ERROR: Could not close fd '%s': errno=%d (%s).\n", fdName, errno,
                      strerror(errno));
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(networkStatusPollTimer);

    EventLoop_Close(eventLoop);

    Log_Debug("INFO: Closing file descriptors\n");

    // Leave the LEDs off
    SetLedStates(false, false, false);

    CloseFdAndPrintError(releaseIpButtonGpioFd, "releaseIpButtonGpioFd");
    CloseFdAndPrintError(renewIpButtonGpioFd, "renewIpButtonGpioFd");
    CloseFdAndPrintError(ipUnassignedLedRedGpioFd, "ipUnassignedLedRedGpioFd");
    CloseFdAndPrintError(ipAssignedLedBlueGpioFd, "ipAssignedLedBlueGpioFd");
    CloseFdAndPrintError(inetAvailableLedGreenGpioFd, "inetAvailavleLedGreenGpioFd");
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>ExitCode_Success if all resources were allocated successfully; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it.
    // This is used to Release the current IP address.
    releaseIpButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (releaseIpButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2Open;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input, and set up a timer to poll it.
    // This is used to Renew the current IP address.
    renewIpButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (renewIpButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2Open;
    }

    // Open LEDs file descriptors, for showing the network interface status.
    ipUnassignedLedRedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (ipUnassignedLedRedGpioFd == -1) {
        Log_Debug("ERROR: Could not open Red LED as output: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_RedLed;
    }
    ipAssignedLedBlueGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (ipAssignedLedBlueGpioFd == -1) {
        Log_Debug("ERROR: Could not open Blue LED as output: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_GreenLed;
    }
    inetAvailableLedGreenGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (inetAvailableLedGreenGpioFd == -1) {
        Log_Debug("ERROR: Could not open Green LED as output: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_BlueLed;
    }

    // Create the event-loop for handling button presses and networking events.
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("ERROR: Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Setup the timer for checking button presses.
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        Log_Debug(
            "ERROR: Could not create periodic timer for "
            "CreateEventLoopPeriodicTimer.\n");
        return ExitCode_Init_ButtonPollTimer;
    }

    // Setup the timer for checking the network interface status.
    networkStatusPollTimer = CreateEventLoopPeriodicTimer(
        eventLoop, &NetworkConnectionStatusTimerEventHandler, &networkStatusCheckPeriod);
    if (networkStatusPollTimer == NULL) {
        Log_Debug(
            "ERROR: Could not create periodic timer for "
            "NetworkConnectionStatusTimerEventHandler.\n");
        return ExitCode_Init_CreateIpSyncStatusTimer;
    }

    Log_Debug("INFO: Successfully initiated peripherals.\n");

    return ExitCode_Success;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("INFO: DHCP High Level Application starting.\n");

    exitCode = InitPeripheralsAndHandlers();

    // Main App loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
            Log_Debug("Error: Eventloop failed with error code: %d errno=%d (%s).\n", result, errno,
                      strerror(errno));
        }
    }

    // Close peripherals and turn the LEDs off.
    ClosePeripheralsAndHandlers();

    Log_Debug("INFO: DHCP High Level Application exiting...\n");
    return exitCode;
}
