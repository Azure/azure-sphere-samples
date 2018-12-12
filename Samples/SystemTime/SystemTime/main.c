/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

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
#include <applibs/wificonfig.h>

#include "mt3620_rdb.h"

// This sample C application for the MT3620 Reference Development Board (Azure Sphere)
// manages the system time and the hardware real-time clock (RTC).
// The system time is changed whenever button A is pressed and it is synchronized with
// the hardware RTC whenever button B is pressed.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - rtc (synchronizes the hardware RTC to the current system time)
// - wificonfig (functions that retrieve the Wi-Fi network configurations on a device)

// File descriptors - initialized to invalid value
static int gpioButtonAFd = -1;
static int gpioButtonBFd = -1;
static int gpioButtonTimerFd = -1;
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
///     Handle button timer event: if button A is pressed, then the current time will be incremented
///     by 3 hours. If button B is pressed, then the current time will be synchronized with the
///     hardware RTC.
/// </summary>
static void ButtonTimerEventHandler(event_data_t *eventData)
{
    if (ConsumeTimerFdEvent(gpioButtonTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // Check for button A press: the changes will not be synchronized with the hardware RTC until
    // button B is pressed
    static GPIO_Value_Type newButtonAState;
    if (IsButtonPressed(gpioButtonAFd, &newButtonAState)) {
        Log_Debug(
            "\nButton A was pressed: the current system time will be incremented by 3 hours. To "
            "synchronize the time with the hardware RTC, press button B.\n");
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

    // Check for button B press: the changes will be synchronized with the hardware RTC
    static GPIO_Value_Type newButtonBState;
    if (IsButtonPressed(gpioButtonBFd, &newButtonBState)) {
        Log_Debug(
            "\nButton B was pressed: the current system time will be synchronized to the "
            "hardware RTC.\n");
        if (clock_systohc() == -1) {
            Log_Debug("ERROR: clock_systohc failed with error code: %s (%d).\n", strerror(errno),
                      errno);
            terminationRequired = true;
        }
    }
}

// Event handler data structures. Only the event handler field needs to be populated.
static event_data_t buttonEventData = {.eventHandler = &ButtonTimerEventHandler};

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

    // Open button A GPIO as input
    Log_Debug("Opening MT3620_RDB_BUTTON_A as input.\n");
    gpioButtonAFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (gpioButtonAFd < 0) {
        Log_Debug("ERROR: Could not open button A GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open button B GPIO as input
    Log_Debug("Opening MT3620_RDB_BUTTON_B as input.\n");
    gpioButtonBFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_B);
    if (gpioButtonBFd < 0) {
        Log_Debug("ERROR: Could not open button B GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Set up a timer to poll the buttons
    struct timespec buttonPressCheckPeriod = {0, 1000000};
    gpioButtonTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonEventData, EPOLLIN);
    if (gpioButtonTimerFd < 0) {
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
    CloseFdAndPrintError(gpioButtonAFd, "GpioButtonA");
    CloseFdAndPrintError(gpioButtonBFd, "GpioButtonB");
    CloseFdAndPrintError(gpioButtonTimerFd, "ButtonTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Check if any Wi-Fi network is enabled on the device. If there are Wi-Fi networks
///     enabled, the current time may be overwritten by NTP.
/// </summary>
static void CheckDeviceConnectivity(void)
{
    ssize_t storedNetworkCount = WifiConfig_GetStoredNetworkCount();
    if (storedNetworkCount == -1) {
        Log_Debug("ERROR: Get stored network count failed: %s (%d).\n", strerror(errno), errno);
        return;
    }

    // If there are stored networks enabled, NTP can reset the time
    if (storedNetworkCount > 0) {
        Log_Debug(
            "Wi-Fi networks are currently configured. This means the current time may be "
            "overwritten by NTP.\nIn order to use this sample to test manual system "
            "time control, you may wish to disable or delete the provided Wi-Fi networks.\n");
    } else {
        Log_Debug(
            "No Wi-Fi networks are configured. The current time will not be overwritten by NTP.\n");
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
        CheckDeviceConnectivity();
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
