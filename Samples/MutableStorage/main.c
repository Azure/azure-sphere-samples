/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere illustrates how to use mutable storage.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - gpio (digital input for buttons)
// - storage (managing persistent user data)
// - eventloop (system invokes handlers for timer events)

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
#include <applibs/eventloop.h>

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

    ExitCode_WriteFile_OpenMutableFile = 2,
    ExitCode_WriteFile_Write = 3,

    ExitCode_ReadFile_OpenMutableFile = 4,
    ExitCode_ReadFile_Read = 5,

    ExitCode_IsButtonPressed_GetValue = 6,

    ExitCode_ButtonTimer_Consume = 7,

    ExitCode_Init_EventLoop = 8,
    ExitCode_Init_OpenUpdateButton = 9,
    ExitCode_Init_OpenDeleteButton = 10,
    ExitCode_Init_OpenLed = 11,
    ExitCode_Init_ButtonTimer = 12,

    ExitCode_Main_EventLoopFail = 13
} ExitCode;

// File descriptors - initialized to invalid value
// Buttons
static int triggerUpdateButtonGpioFd = -1;
static int triggerDeleteButtonGpioFd = -1;

// LEDs
static int appRunningLedFd = -1;

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// Button state variables
static GPIO_Value_Type triggerUpdateButtonState = GPIO_Value_High;
static GPIO_Value_Type triggerDeleteButtonState = GPIO_Value_High;

static void TerminationHandler(int signalNumber);
static void WriteToMutableFile(int value);
static int ReadMutableFile(void);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void UpdateButtonHandler(void);
static void DeleteButtonHandler(void);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

static volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
/// Write an integer to this application's persistent data file
/// </summary>
static void WriteToMutableFile(int value)
{
    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_WriteFile_OpenMutableFile;
        return;
    }
    ssize_t ret = write(fd, &value, sizeof(value));
    if (ret == -1) {
        // If the file has reached the maximum size specified in the application manifest,
        // then -1 will be returned with errno EDQUOT (122)
        Log_Debug("ERROR: An error occurred while writing to mutable file:  %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_WriteFile_Write;
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
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ReadFile_OpenMutableFile;
        return -1;
    }
    int value = 0;
    ssize_t ret = read(fd, &value, sizeof(value));
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while reading file:  %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_ReadFile_Read;
    }
    close(fd);

    if (ret < sizeof(value)) {
        return 0;
    }

    return value;
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
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
/// Pressing SAMPLE_BUTTON_1 will:
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
/// Pressing SAMPLE_BUTTON_2 will delete the user file
/// </summary>
static void DeleteButtonHandler(void)
{
    if (IsButtonPressed(triggerDeleteButtonGpioFd, &triggerDeleteButtonState)) {
        int ret = Storage_DeleteMutableFile();
        if (ret == -1) {
            Log_Debug("An error occurred while deleting the mutable file: %s (%d).\n",
                      strerror(errno), errno);
        } else {
            Log_Debug("Successfully deleted the mutable file!\n");
        }
    }
}

/// <summary>
/// Button timer event:  Check the status of the buttons.
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }
    UpdateButtonHandler();
    DeleteButtonHandler();
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
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
    Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
    triggerUpdateButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (triggerUpdateButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OpenUpdateButton;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input\n");
    triggerDeleteButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (triggerDeleteButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OpenDeleteButton;
    }

    // Turn SAMPLE_LED on for a visible sign that this application is loaded on the device
    // This isn't critical for the operation of this app. If your hardware doesn't have an
    // on-board LED, there is no need to wire one up.
    Log_Debug("Opening SAMPLE_LED as output\n");
    appRunningLedFd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (appRunningLedFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_LED: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OpenLed;
    }

    // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod100Ms = {.tv_sec = 0,
                                                                .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod100Ms);
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

    Log_Debug("Closing file descriptors\n");

    // Leave the LEDs off
    if (appRunningLedFd >= 0) {
        GPIO_SetValue(appRunningLedFd, GPIO_Value_High);
    }

    CloseFdAndPrintError(triggerUpdateButtonGpioFd, "TriggerUpdateButtonGpio");
    CloseFdAndPrintError(triggerDeleteButtonGpioFd, "TriggerDeleteButtonGpio");
    CloseFdAndPrintError(appRunningLedFd, "AppRunningLedBlueGpio");
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Mutable storage application starting\n");
    Log_Debug("Press SAMPLE_BUTTON_1 to write to file, and SAMPLE_BUTTON_2 to delete the file\n");

    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting\n");
    return exitCode;
}
