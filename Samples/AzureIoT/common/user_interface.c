/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>

#include <applibs/gpio.h>
#include <applibs/log.h>

#include "user_interface.h"

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

static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void CloseFdAndPrintError(int fd, const char *fdName);

// File descriptors - initialized to invalid value
static int buttonAGpioFd = -1;
static int buttonBGpioFd = -1;
static int statusLedGpioFd = -1;

static EventLoopTimer *buttonPollTimer = NULL;

static ExitCode_CallbackType failureCallbackFunction = NULL;
static UserInterface_ButtonPressedCallbackType buttonPressedCallbackFunction = NULL;

// State variables
static GPIO_Value_Type buttonAState = GPIO_Value_High;
static GPIO_Value_Type buttonBState = GPIO_Value_High;

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
        failureCallbackFunction(ExitCode_IsButtonPressed_GetValue);
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Button timer event:  Check the status of the button
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        failureCallbackFunction(ExitCode_ButtonTimer_Consume);
        return;
    }

    if (IsButtonPressed(buttonAGpioFd, &buttonAState) && NULL != buttonPressedCallbackFunction) {
        buttonPressedCallbackFunction(UserInterface_Button_A);
    }

    if (IsButtonPressed(buttonBGpioFd, &buttonBState) && NULL != buttonPressedCallbackFunction) {
        buttonPressedCallbackFunction(UserInterface_Button_B);
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

ExitCode UserInterface_Initialise(EventLoop *el,
                                  UserInterface_ButtonPressedCallbackType buttonPressedCallback,
                                  ExitCode_CallbackType failureCallback)
{
    failureCallbackFunction = failureCallback;
    buttonPressedCallbackFunction = buttonPressedCallback;

    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    buttonAGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonAGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    buttonBGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (buttonBGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }

    // SAMPLE_LED is used to show state
    Log_Debug("Opening SAMPLE_LED as output.\n");
    statusLedGpioFd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (statusLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_LED: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Led;
    }

    // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(el, &ButtonPollTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    return ExitCode_Success;
}

void UserInterface_Cleanup(void)
{
    DisposeEventLoopTimer(buttonPollTimer);

    // Leave the LEDs off
    if (statusLedGpioFd >= 0) {
        GPIO_SetValue(statusLedGpioFd, GPIO_Value_High);
    }

    CloseFdAndPrintError(buttonAGpioFd, "ButtonA");
    CloseFdAndPrintError(buttonBGpioFd, "ButtonB");
    CloseFdAndPrintError(statusLedGpioFd, "StatusLed");
}

void UserInterface_SetStatus(bool status)
{
    GPIO_SetValue(statusLedGpioFd, status ? GPIO_Value_Low : GPIO_Value_High);
}