/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere manages the system time and the hardware
// real-time clock (RTC). The system time is changed whenever SAMPLE_BUTTON_1 is pressed
// and it is synchronized with the hardware RTC whenever SAMPLE_BUTTON_2 is pressed.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (displays messages in the Device Output window during debugging)
// - rtc (synchronizes the hardware RTC to the current system time)
// - wificonfig (functions that retrieve the Wi-Fi network configurations on a device)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/rtc.h>
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

    ExitCode_PrintTime_ClockGetTime = 2,
    ExitCode_PrintTime_UtcTimeR = 3,
    ExitCode_PrintTime_LocalTimeR = 4,

    ExitCode_IsButtonPressed_GetValue = 5,

    ExitCode_ButtonTimer_Consume = 6,
    ExitCode_ButtonTimer_GetTime = 7,
    ExitCode_ButtonTimer_SetTime = 8,
    ExitCode_ButtonTimer_SysToHc = 9,

    ExitCode_Init_EventLoop = 10,
    ExitCode_Init_Button1Open = 11,
    ExitCode_Init_Button2Open = 12,
    ExitCode_Init_ButtonTimer = 13,

    ExitCode_Main_SetEnv = 14,
    ExitCode_Main_EventLoopFail = 15
} ExitCode;

// File descriptors - initialized to invalid value
static int incrementTimeButtonGpioFd = -1;
static int writeToRtcButtonGpioFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void PrintTime(void);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);
static void CheckTimeSyncState(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Print the time in both UTC time zone and local time zone.
/// </summary>
static void PrintTime(void)
{
    // Ask for CLOCK_REALTIME to obtain the current system time. This is not to be confused with the
    // hardware RTC used below to persist the time.
    struct timespec currentTime;
    if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
        Log_Debug("ERROR: clock_gettime failed with error code: %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_PrintTime_ClockGetTime;
        return;
    } else {
        char displayTimeBuffer[26];
        if (!asctime_r((gmtime(&currentTime.tv_sec)), (char *restrict) & displayTimeBuffer)) {
            Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_PrintTime_UtcTimeR;
            return;
        }
        Log_Debug("UTC:            %s", displayTimeBuffer);

        if (!asctime_r((localtime(&currentTime.tv_sec)), (char *restrict) & displayTimeBuffer)) {
            Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_PrintTime_LocalTimeR;
            return;
        }

        // Remove the new line at the end of 'displayTimeBuffer'
        displayTimeBuffer[strlen(displayTimeBuffer) - 1] = '\0';
        size_t tznameIndex = ((localtime(&currentTime.tv_sec))->tm_isdst) ? 1 : 0;
        Log_Debug("Local time:     %s %s\n", displayTimeBuffer, tzname[tznameIndex]);
    }
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>True if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Handle button timer event: if SAMPLE_BUTTON_1 is pressed, then the current time will be
///     incremented by 3 hours. If SAMPLE_BUTTON_2 is pressed, then the current time will be
///     synchronized with the hardware RTC.
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for advance-clock button press; the changes will not be synchronized with the hardware
    // RTC until the other button is pressed
    static GPIO_Value_Type incrementTimeButtonState;
    if (IsButtonPressed(incrementTimeButtonGpioFd, &incrementTimeButtonState)) {
        Log_Debug(
            "\nSAMPLE_BUTTON_1 was pressed: the current system time will be incremented by 3 hours."
            "To synchronize the time with the hardware RTC, press SAMPLE_BUTTON_2.\n");
        struct timespec currentTime;
        if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
            Log_Debug("ERROR: clock_gettime failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_ButtonTimer_GetTime;
            return;
        }

        // Add three hours to the current time
        currentTime.tv_sec += 10800;
        if (clock_settime(CLOCK_REALTIME, &currentTime) == -1) {
            Log_Debug("ERROR: clock_settime failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_ButtonTimer_SetTime;
            return;
        }
        PrintTime();
    }

    // Check for SAMPLE_BUTTON_2 press: the changes will be synchronized with the hardware RTC
    static GPIO_Value_Type writeToRtcButtonState;
    if (IsButtonPressed(writeToRtcButtonGpioFd, &writeToRtcButtonState)) {
        Log_Debug(
            "\nSAMPLE_BUTTON_2 was pressed: the current system time will be synchronized to the "
            "hardware RTC.\n");
        if (clock_systohc() == -1) {
            Log_Debug("ERROR: clock_systohc failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_ButtonTimer_SysToHc;
        }
    }
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

    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    incrementTimeButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (incrementTimeButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button1Open;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    writeToRtcButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (writeToRtcButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2Open;
    }

    // Set up a timer to poll the buttons
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonTimer;
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
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(incrementTimeButtonGpioFd, "IncrementTimeButtonGpio");
    CloseFdAndPrintError(writeToRtcButtonGpioFd, "WriteToRtcButtonGpio");
}

/// <summary>
///     Check whether time sync is enabled on the device. If it is enabled, the current time may be
///     overwritten by NTP.
/// </summary>
static void CheckTimeSyncState(void)
{
    bool isTimeSyncEnabled = false;
    int result = Networking_TimeSync_GetEnabled(&isTimeSyncEnabled);
    if (result != 0) {
        Log_Debug("ERROR: Networking_TimeSync_GetEnabled failed: %s (%d).\n", strerror(errno),
                  errno);
        return;
    }

    // If time sync is enabled, NTP can reset the time
    if (isTimeSyncEnabled) {
        Log_Debug(
            "The device's NTP time-sync service is enabled. This means the current time may be "
            "overwritten by NTP.\nIn order to use this sample to test manual system "
            "time control, you may wish to ensure the device isn't connected to the internet.\n");
    } else {
        Log_Debug(
            "NTP time-sync service is disabled on the device. The current time will not be "
            "overwritten by NTP.\n");
    }
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("System time application starting.\n");
    exitCode = InitPeripheralsAndHandlers();

    if (exitCode == ExitCode_Success) {
        CheckTimeSyncState();
        Log_Debug("\nTime before setting time zone:\n");
        PrintTime();
        // Note that the offset is positive if the local time zone is west of the Prime Meridian and
        // negative if it is east.
        Log_Debug("\nSetting local time zone to: PST+8:\n");
        int result = setenv("TZ", "PST+8", 1);
        if (result == -1) {
            Log_Debug("ERROR: setenv failed with error code: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_Main_SetEnv;
        } else {
            tzset();
            PrintTime();
        }
    }

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}
