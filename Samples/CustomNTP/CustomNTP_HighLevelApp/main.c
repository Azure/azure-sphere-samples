/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the custom NTP server APIs.
// It shows how to:
// 1. Configure the default NTP server.
// 2. Configure the automatic NTP server.
// 3. Configure up to two custom NTP servers.
// 4. Get the last NTP sync information.

// It uses the API for the following Azure Sphere application libraries:
// - eventloop (system invokes handlers for timer events).
// - gpio (digital input for button, digital output for LED).
// - log (displays messages in the Device Output window during debugging).
// - networking (functions to configure the NTP and retrieve last synced
// NTP information).

// You will need to provide information in the 'CmdArgs' section of the application manifest to
// use this application. Please see README.md for full details.

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    ExitCode_Init_GreenLed = 10,
    ExitCode_Init_CreateNtpSyncStatusTimer = 11,

    ExitCode_TimeSync_DefaultNtp_Failed = 12,
    ExitCode_TimeSync_AutomaticNtp_Failed = 13,
    ExitCode_TimeSync_CustomNtp_Failed = 14,
    ExitCode_TimeSync_GetLastSyncInfo_Failed = 15,
    ExitCode_TimeSync_SetEnabled_Failed = 16,

    ExitCode_IsNetworkingReady_Failed = 17,
    ExitCode_IsNetworkingReady_NetworkNotReady = 18, // This enumerated constant is not used.

    ExitCode_Validate_TimeSource = 19,
    ExitCode_Validate_PrimaryNtpServer = 20,
    ExitCode_Validate_SecondaryNtpServer = 21,

    ExitCode_Main_SetEnv = 22,
    ExitCode_Main_EventLoopFail = 23
} ExitCode;

/// <summary>
/// Time source to use when configuring NTP server.
/// </summary>
typedef enum {
    TimeSource_NotDefined = 0,
    TimeSource_Default = 1,
    TimeSource_Automatic = 2,
    TimeSource_Custom = 3
} TimeSource;

// User configuration.
static char *primaryNtpServer = NULL;                 // Primary server for custom NTP.
static char *secondaryNtpServer = NULL;               // Secondary server for custom NTP.
static TimeSource timeSource = TimeSource_NotDefined; // Time source.
static Networking_NtpOption fallbackServerNtpOption = Networking_NtpOption_FallbackServerEnabled;

// File descriptors - initialized to invalid value.
static int getLastNtpSyncInfoButtonGpioFd = -1;

// The status mode LED shows whether the application has sucessfully time synced
// with the NTP server (SAMPLE_RGBLED_GREEN) or not (SAMPLE_RGBLED_RED).
static int ntpNotSyncedLedRedGpioFd = -1;
static int ntpSyncedLedGreenGpioFd = -1;

// Button state variables.
static GPIO_Value_Type getLastNtpSyncInfoButtonState = GPIO_Value_High;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// The NTP sync status is checked every second.
static EventLoopTimer *ntpSyncStatusTimer = NULL;
static const struct timespec ntpSyncStatusTimerInterval = {.tv_sec = 1, .tv_nsec = 0};

// Termination state.
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static void NtpSyncStatusTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

static void ConfigureNtpServer(void);
static void ConfigureAutomaticNtpServer(void);
static void ConfigureCustomNtpServer(void);
static void ConfigureDefaultNtpServer(void);
static void GetLastNtpSyncInformation(void);
static bool IsNetworkReady(void);
static void EnableTimeSyncService(void);
static void ParseCommandLineArguments(int argc, char *argv[]);
static ExitCode ValidateUserConfiguration(void);

// State variables.
static bool networkReady = false;

// Usage text for command-line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "\nDefault NTP Server: \" CmdArgs \": [\"--TimeSource\", \"Default\"]\n"
    "Automatic NTP Server: \" CmdArgs \": [\"--TimeSource\", \"Automatic\"]\n"
    "Custom NTP Server: \" CmdArgs \": [\"--TimeSource\", \"Custom\", \"--PrimaryNtpServer\", "
    "\"<hostname_or_ip>\", \"--SecondaryNtpServer\", \"<hostname_or_ip>\"]\n\n"
    "To disable the fallback (default) server for Automatic or Custom time source, include option "
    "\"--DisableFallback\" with no argument.\n";

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
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
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *buttonState) && (newState == GPIO_Value_Low);
        *buttonState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Button timer event:  Check the status of the buttons.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check if BUTTON_1 was pressed.
    bool isButtonPressed =
        IsButtonPressed(getLastNtpSyncInfoButtonGpioFd, &getLastNtpSyncInfoButtonState);
    if (isButtonPressed) {
        GetLastNtpSyncInformation();
    }
}

/// <summary>
///     NTP sync status timer: Checks the NTP sync status of the device.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
static void NtpSyncStatusTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_SyncStatusTimer_Consume;
        return;
    }

    bool currentNetworkingReady = IsNetworkReady();

    if (currentNetworkingReady == false) {
        Log_Debug(
            "Device has not yet successfully time synced. Ensure that the network interface is "
            "up and the configured NTP server is valid.\n");
    }

    if (currentNetworkingReady != networkReady) {
        // Toggle the states.
        networkReady = currentNetworkingReady;
        if (networkReady) {
            // Network is ready. Turn off Red LED. Turn on Green LED.
            GPIO_SetValue(ntpNotSyncedLedRedGpioFd, GPIO_Value_High);
            GPIO_SetValue(ntpSyncedLedGreenGpioFd, GPIO_Value_Low);
            Log_Debug("Successfully synced with the configured NTP server.\n");
            return;
        }

        // Network is not ready. Turn on Red LED. Turn off Green LED.
        GPIO_SetValue(ntpSyncedLedGreenGpioFd, GPIO_Value_High);
        GPIO_SetValue(ntpNotSyncedLedRedGpioFd, GPIO_Value_Low);
    }
}

/// <summary>
///     Retrieves the last NTP sync information.
/// </summary>
static void GetLastNtpSyncInformation(void)
{
    if (!networkReady) {
        Log_Debug("Device has not yet successfully time synced.\n");
        return;
    }

    size_t ntpServerLength = 256;
    char ntpServer[ntpServerLength];
    struct tm timeBeforeSync;
    struct tm adjustedNtpTime;

    if (Networking_TimeSync_GetLastNtpSyncInfo(ntpServer, &ntpServerLength, &timeBeforeSync,
                                               &adjustedNtpTime) == -1) {

        if (errno == ENOENT) {
            Log_Debug("INFO: The device has not yet successfully completed a time sync.\n");
            return;
        }

        if (errno == ENOBUFS) {
            Log_Debug("ERROR: Buffer is too small to hold the NTP server. Size required is %zu\n",
                      ntpServerLength);
        }

        Log_Debug("ERROR: Get last NTP sync info failed: %d (%s)\n", errno, strerror(errno));

        exitCode = ExitCode_TimeSync_GetLastSyncInfo_Failed;
        return;
    }

    Log_Debug("\nSuccessfully time synced to server %s\n", ntpServer);

    char displayTimeBuffer[26];

    if (strftime(displayTimeBuffer, sizeof(displayTimeBuffer), "%c", &timeBeforeSync) != 0) {
        Log_Debug("\nTime before sync:\n");
        Log_Debug("UTC time        : %s\n", displayTimeBuffer);
    }

    if (strftime(displayTimeBuffer, sizeof(displayTimeBuffer), "%c", &adjustedNtpTime) != 0) {
        Log_Debug("\nTime after sync:\n");
        Log_Debug("UTC time        : %s\n", displayTimeBuffer);
    }
}

/// <summary>
///     Configures automatic NTP server.
/// </summary>
static void ConfigureAutomaticNtpServer(void)
{
    Log_Debug("\nConfiguring Automatic NTP server\n");
    Log_Debug("Fallback Server NTP Option: %d\n", fallbackServerNtpOption);

    if (Networking_TimeSync_EnableAutomaticNtp(fallbackServerNtpOption) == -1) {
        Log_Debug("ERROR: Configure Automatic NTP failed: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_AutomaticNtp_Failed;
    }
}

/// <summary>
///     Configures custom NTP server. Sets the exit code if it encounters any error.
/// </summary>
static void ConfigureCustomNtpServer(void)
{
    Log_Debug("\nConfiguring Custom NTP server\n");
    Log_Debug("Primary Server: %s\n", primaryNtpServer);
    if (secondaryNtpServer != NULL) {
        Log_Debug("Secondary Server: %s\n", secondaryNtpServer);
    }
    Log_Debug("Fallback Server NTP Option: %d\n", fallbackServerNtpOption);

    if (Networking_TimeSync_EnableCustomNtp(primaryNtpServer, secondaryNtpServer,
                                            fallbackServerNtpOption) == -1) {
        Log_Debug("ERROR: Configure Custom NTP failed: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_CustomNtp_Failed;
    }
}

/// <summary>
///     Configures default NTP server. Sets the exit code if it encounters any error.
/// </summary>
static void ConfigureDefaultNtpServer(void)
{
    Log_Debug("\nConfiguring Default NTP server\n");

    if (Networking_TimeSync_EnableDefaultNtp() == -1) {
        Log_Debug("ERROR: Configure Automatic NTP failed: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_DefaultNtp_Failed;
    }
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
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(ntpSyncStatusTimer);

    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");

    // Leave the LEDs off
    if (ntpNotSyncedLedRedGpioFd != -1) {
        GPIO_SetValue(ntpNotSyncedLedRedGpioFd, GPIO_Value_High);
    }
    if (ntpSyncedLedGreenGpioFd != -1) {
        GPIO_SetValue(ntpSyncedLedGreenGpioFd, GPIO_Value_High);
    }

    CloseFdAndPrintError(getLastNtpSyncInfoButtonGpioFd, "GetLastNtpSyncInfoButton");
    CloseFdAndPrintError(ntpNotSyncedLedRedGpioFd, "StatusLedRed");
    CloseFdAndPrintError(ntpSyncedLedGreenGpioFd, "StatusLedGreen");
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

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it.
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    getLastNtpSyncInfoButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (getLastNtpSyncInfoButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2Open;
    }

    static const struct timespec buttonPressCheckPeriod100Ms = {.tv_sec = 0,
                                                                .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod100Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // Open LEDs for NTP sync status.
    // Turn on Red LED at startup, till we get a successful sync.
    Log_Debug("Opening SAMPLE_LED as output.\n");
    ntpNotSyncedLedRedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (ntpNotSyncedLedRedGpioFd == -1) {
        Log_Debug("ERROR: Could not open Red LED as output: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_RedLed;
    }

    ntpSyncedLedGreenGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (ntpSyncedLedGreenGpioFd == -1) {
        Log_Debug("ERROR: Could not open Green LED as output: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_GreenLed;
    }

    ntpSyncStatusTimer = CreateEventLoopPeriodicTimer(eventLoop, &NtpSyncStatusTimerEventHandler,
                                                      &ntpSyncStatusTimerInterval);
    if (ntpSyncStatusTimer == NULL) {
        return ExitCode_Init_CreateNtpSyncStatusTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Validates that the values of the time source, fallback, primary and secondary NTP servers
///     were set.
/// </summary>
/// <returns>ExitCode_Success if the parameters were provided; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (timeSource < TimeSource_Default || timeSource > TimeSource_Custom) {
        validationExitCode = ExitCode_Validate_TimeSource;
    }

    if (timeSource == TimeSource_Custom) {
        if (primaryNtpServer == NULL) {
            validationExitCode = ExitCode_Validate_PrimaryNtpServer;
        }
        // Secondary NTP server is optional. Hence it is not validated.
    }

    if (validationExitCode != ExitCode_Success) {
        Log_Debug("Command-line arguments for application should be set as below\n%s",
                  cmdLineArgsUsageText);
    }

    return validationExitCode;
}

/// <summary>
///     Parse the command-line arguments given in the application manifest.
/// </summary>
static void ParseCommandLineArguments(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "TimeSource", .has_arg = required_argument, .flag = NULL, .val = 't'},
        {.name = "PrimaryNtpServer", .has_arg = required_argument, .flag = NULL, .val = 'p'},
        {.name = "SecondaryNtpServer", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = "DisableFallback", .has_arg = no_argument, .flag = NULL, .val = 'f'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "t:p:s:f", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option except 'DisableFallback' requires an
        // argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 't':
            Log_Debug("TimeSource: %s\n", optarg);
            if (strcmp(optarg, "Default") == 0) {
                timeSource = TimeSource_Default;
            } else if (strcmp(optarg, "Automatic") == 0) {
                timeSource = TimeSource_Automatic;
            } else if (strcmp(optarg, "Custom") == 0) {
                timeSource = TimeSource_Custom;
            }
            break;
        case 'p':
            Log_Debug("PrimaryNtpServer: %s\n", optarg);
            primaryNtpServer = optarg;
            break;
        case 's':
            Log_Debug("SecondaryNtpServer: %s\n", optarg);
            secondaryNtpServer = optarg;
            break;
        case 'f':
            fallbackServerNtpOption = Networking_NtpOption_FallbackServerDisabled;
            Log_Debug("Fallback NTP server disabled.\n");
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }
}

/// <summary>
///     Checks if the network is ready. It logs an error and sets the exit code
///     if it encounters any error or if the network is not ready.
/// </summary>
static bool IsNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_IsNetworkingReady_Failed;
        return false;
    }

    return isNetworkReady;
}

/// <summary>
///     Enables the time sync service. It logs an error and sets the exit code if it encounters any
///     error.
/// </summary>
static void EnableTimeSyncService(void)
{
    if (Networking_TimeSync_SetEnabled(true) == -1) {
        Log_Debug("ERROR: Networking_TimeSync_SetEnabled: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_SetEnabled_Failed;
    }
}

/// <summary>
///     Checks that the internet is connected, enables time sync service and configures the NTP
///     server based on the user configuration.
/// </summary>
static void ConfigureNtpServer(void)
{
    EnableTimeSyncService();
    if (exitCode != ExitCode_Success) {
        return;
    }

    switch (timeSource) {
    case TimeSource_Default:
        ConfigureDefaultNtpServer();
        break;
    case TimeSource_Automatic:
        ConfigureAutomaticNtpServer();
        break;
    case TimeSource_Custom:
        ConfigureCustomNtpServer();
        break;
    default:
        Log_Debug("Unknown time source: %d \n", timeSource);
        break;
    }
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("INFO: Custom NTP High Level Application starting.\n");

    ParseCommandLineArguments(argc, argv);

    exitCode = ValidateUserConfiguration();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    exitCode = InitPeripheralsAndHandlers();

    ConfigureNtpServer();

    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
            Log_Debug("Error: eventloop failed with error code: %d %d %s\n", result, errno,
                      strerror(errno));
        }
    }

    // Close peripherals and turn the LEDs off.
    ClosePeripheralsAndHandlers();

    Log_Debug("INFO: Custom NTP High Level Application exiting...\n");
    return exitCode;
}
