/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere manages the system time and the hardware
// real-time clock (RTC). The system time is changed whenever SAMPLE_BUTTON_1 is pressed
// and it is synchronized with the hardware RTC whenever SAMPLE_BUTTON_2 is pressed.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - rtc (synchronizes the hardware RTC to the current system time)
// - wificonfig (functions that retrieve the Wi-Fi network configurations on a device)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"

#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/rtc.h>
#include <applibs/networking.h>

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed using the project property "Target Hardware Definition Directory".
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

// File descriptors - initialized to invalid value
static int incrementTimeButtonGpioFd = -1;
static int writeToRtcButtonGpioFd = -1;
static int buttonPollTimerFd = -1;
static int epollFd = -1;

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
        terminationRequired = true;
        return;
    } else {
        char displayTimeBuffer[26];
        if (!asctime_r((gmtime(&currentTime.tv_sec)), (char *restrict) & displayTimeBuffer)) {
            Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            terminationRequired = true;
            return;
        }
        Log_Debug("UTC:            %s", displayTimeBuffer);

        if (!asctime_r((localtime(&currentTime.tv_sec)), (char *restrict) & displayTimeBuffer)) {
            Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            terminationRequired = true;
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
        terminationRequired = true;
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
static void ButtonPollTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) {
        terminationRequired = true;
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
            terminationRequired = true;
            return;
        }

        // Add three hours to the current time
        currentTime.tv_sec += 10800;
        if (clock_settime(CLOCK_REALTIME, &currentTime) == -1) {
            Log_Debug("ERROR: clock_settime failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            terminationRequired = true;
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
            terminationRequired = true;
        }
    }
}

// Event handler data structures. Only the event handler field needs to be populated.
static EventData buttonPollTimerEventData = {.eventHandler = &ButtonPollTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    incrementTimeButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (incrementTimeButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    writeToRtcButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (writeToRtcButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Set up a timer to poll the buttons
    struct timespec buttonPressCheckPeriod = {0, 1000000};
    buttonPollTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod,
                                                   &buttonPollTimerEventData, EPOLLIN);
    if (buttonPollTimerFd < 0) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(incrementTimeButtonGpioFd, "IncrementTimeButtonGpio");
    CloseFdAndPrintError(writeToRtcButtonGpioFd, "WriteToRtcButtonGpio");
    CloseFdAndPrintError(buttonPollTimerFd, "ButtonPollTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
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
    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    if (!terminationRequired) {
        CheckTimeSyncState();
        Log_Debug("\nTime before setting time zone:\n");
        PrintTime();
        // Note that the offset is positive if the local time zone is west of the Prime Meridian and
        // negative if it is east.
        Log_Debug("\nSetting local time zone to: PST+8:\n");
        int result = setenv("TZ", "PST+8", 1);
        if (result == -1) {
            Log_Debug("ERROR: setenv failed with error code: %s (%d).\n", strerror(errno), errno);
            terminationRequired = true;
        } else {
            tzset();
            PrintTime();
        }
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return 0;
}
