/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere illustrates how to use mutable storage.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - gpio (digital input for buttons)
// - storage (managing persistent user data)

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/gpio.h>

#include "epoll_timerfd_utilities.h"

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed using the project property "Target Hardware Definition Directory".
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

// File descriptors - initialized to invalid value
// Buttons
static int triggerUpdateButtonGpioFd = -1;
static int triggerDeleteButtonGpioFd = -1;

// LEDs
static int appRunningLedBlueGpioFd = -1;
static int appRunningLedRedGpioFd = -1;

// Timer / polling
static int buttonPollTimerFd = -1;
static int epollFd = -1;

// Button state variables
static GPIO_Value_Type triggerUpdateButtonState = GPIO_Value_High;
static GPIO_Value_Type triggerDeleteButtonState = GPIO_Value_High;

/// <summary>
/// Write an integer to this application's persistent data file
/// </summary>
static void WriteToMutableFile(int value)
{
    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        return;
    }
    ssize_t ret = write(fd, &value, sizeof(value));
    if (ret < 0) {
        // If the file has reached the maximum size specified in the application manifest,
        // then -1 will be returned with errno EDQUOT (122)
        Log_Debug("ERROR: An error occurred while writing to mutable file:  %s (%d).\n",
                  strerror(errno), errno);
    } else if (ret < sizeof(value)) {
        // For simplicity, this sample logs an error here. In the general case, this should be
        // handled by retrying the write with the remaining data until all the data has been
        // written.
        Log_Debug("ERROR: Only wrote %d of %d bytes requested\n", ret, (int)sizeof(value));
    }
    close(fd);
}

/// <summary>
/// Read an integer from this application's persistent data file
/// </summary>
/// <returns>
/// The integer that was read from the file.  If the file is empty, this returns 0.  If the storage
/// API fails, this returns -1.
/// </returns>
static int ReadMutableFile(void)
{
    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    int value = 0;
    ssize_t ret = read(fd, &value, sizeof(value));
    if (ret < 0) {
        Log_Debug("ERROR: An error occurred while reading file:  %s (%d).\n", strerror(errno),
                  errno);
    }
    close(fd);

    if (ret < sizeof(value)) {
        return 0;
    }

    return value;
}

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
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if pressed, false otherwise</returns>
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
/// Pressing button A will:
///		- Read from this application's file
///		- If there is data in this file, read it and increment
///		- Write the integer to file
/// </summary>
static void UpdateButtonHandler(void)
{
    if (IsButtonPressed(triggerUpdateButtonGpioFd, &triggerUpdateButtonState)) {
        int readFromFile = ReadMutableFile();
        int writeToFile = readFromFile + 1;

        if (readFromFile <= 0) {
            Log_Debug("Writing %d to the mutable file\n", writeToFile);
        } else {
            Log_Debug("Read %d from the mutable file, updating to %d\n", readFromFile, writeToFile);
        }

        WriteToMutableFile(writeToFile);
    }
}

/// <summary>
/// Pressing button B will delete the user file
/// </summary>
static void DeleteButtonHandler(void)
{
    if (IsButtonPressed(triggerDeleteButtonGpioFd, &triggerDeleteButtonState)) {
        int ret = Storage_DeleteMutableFile();
        if (ret < 0) {
            Log_Debug("An error occurred while deleting the mutable file: %s (%d).\n",
                      strerror(errno), errno);
        } else {
            Log_Debug("Successfully deleted the mutable file!\n");
        }
    }
}

/// <summary>
/// Button timer event:  Check the status of both buttons
/// </summary>
static void ButtonPollTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) {
        terminationRequired = true;
        return;
    }
    UpdateButtonHandler();
    DeleteButtonHandler();
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonPollEventData = {.eventHandler = &ButtonPollTimerEventHandler};

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

    // Open button GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
    triggerUpdateButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (triggerUpdateButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open button A: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open button GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input\n");
    triggerDeleteButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (triggerDeleteButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open button B: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Make LED 4 magenta for a visible sign that this application is loaded on the device
    Log_Debug("Opening SAMPLE_RGBLED_BLUE as output\n");
    appRunningLedBlueGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (appRunningLedBlueGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_RED: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    Log_Debug("Opening SAMPLE_RGBLED_RED as output\n");
    appRunningLedRedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (appRunningLedRedGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_GREEN: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Set up a timer to poll for button events.
    struct timespec buttonPressCheckPeriod = {0, 1000 * 1000};
    buttonPollTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonPollEventData, EPOLLIN);
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
    Log_Debug("Closing file descriptors\n");

    // Leave the LEDs off
    if (appRunningLedBlueGpioFd >= 0) {
        GPIO_SetValue(appRunningLedBlueGpioFd, GPIO_Value_High);
    }
    if (appRunningLedRedGpioFd >= 0) {
        GPIO_SetValue(appRunningLedRedGpioFd, GPIO_Value_High);
    }

    CloseFdAndPrintError(buttonPollTimerFd, "ButtonPollTimer");
    CloseFdAndPrintError(triggerUpdateButtonGpioFd, "TriggerUpdateButtonGpio");
    CloseFdAndPrintError(triggerDeleteButtonGpioFd, "TriggerDeleteButtonGpio");
    CloseFdAndPrintError(appRunningLedBlueGpioFd, "AppRunningLedBlueGpio");
    CloseFdAndPrintError(appRunningLedRedGpioFd, "AppRunningLedRedGpio");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Mutable storage application starting\n");
    Log_Debug("Press Button A to write to file, and Button B to delete the file\n");

    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting\n");
    return 0;
}
