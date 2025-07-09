/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// Do NOT copy and paste this code into your source code as this tutorial
// application intentionally crashes and exits.

// This tutorial C application for Azure Sphere demonstrates error reporting
// techniques using a blinking LED and a button.
// The application can be forced to crash or exit by pressing a button.
//
// It uses the following Azure Sphere application libraries:
// - gpio (digital input for button, digital output for LED)
// - log (messages shown in Visual Studio's and VS Code's Device Output window during debugging)
// - eventloop (system invokes handlers for IO events)
// - networking (network ready)

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
#include <applibs/eventloop.h>
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

// This tutorial uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_LedTimer_Consume = 2,
    ExitCode_LedTimer_SetLedState1 = 3,
    ExitCode_ButtonTimer_Consume = 4,
    ExitCode_ButtonTimer_GetButtonState = 5,
    ExitCode_ButtonTimer_SetLedState = 6,
    ExitCode_Exit_SuccessfulButtonBPress = 7,
    ExitCode_ButtonTimer_SetBlinkPeriod = 8,
    ExitCode_Init_EventLoop = 9,
    ExitCode_Init_Button1 = 10,
    ExitCode_Init_Button2 = 11,
    ExitCode_Init_ButtonPollTimer = 12,
    ExitCode_Init_LedBlue = 13,
    ExitCode_Init_LedBlinkTimer = 14,
    ExitCode_Init_LedGreen = 15,
    ExitCode_Main_EventLoopFail = 16,
    ExitCode_IsNetworkingReady_Failed = 17,
    ExitCode_NetworkReadyCheckHandler_Consume = 18,
    ExitCode_Init_NetworkReadyCheckTimer = 19
} ExitCode;

// EventLoops and timers
static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;
static EventLoopTimer *blinkTimer = NULL;
static EventLoopTimer *networkReadyCheckTimer = NULL;

// File descriptors - initialized to invalid value
static int ledBlinkButton1GpioFd = -1;
static int ledBlinkButton2GpioFd = -1;
static int blinkingLedBlueGpioFd = -1;
static int blinkingLedGreenGpioFd = -1;

// State variables
static GPIO_Value_Type button1State = GPIO_Value_High;
static GPIO_Value_Type button2State = GPIO_Value_High;
static GPIO_Value_Type ledState = GPIO_Value_High;

// Variable responsible for changing the color of the blinking LED
static bool buttonToggle = true;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void BlinkingLedTimerEventHandler(EventLoopTimer *timer);
static void ButtonTimerEventHandler(EventLoopTimer *timer);
static void CheckButtonA(void);
static void CheckButtonB(void);
static void DeferenceNull(void);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static bool IsNetworkReady(void);
static ExitCode InitPeripheralsAndHandlers(void);
static void NetworkReadyCheckTimerEventHandler(EventLoopTimer *timer);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Handle LED timer event: blink LED.
/// </summary>
static void BlinkingLedTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_LedTimer_Consume;
        return;
    }

    // The LED is active-low so GPIO_Value_Low is on and GPIO_Value_High is off
    ledState = (ledState == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);

    // Changes the color of the LED when the button is pressed.
    int result =
        GPIO_SetValue(buttonToggle ? blinkingLedBlueGpioFd : blinkingLedGreenGpioFd, ledState);
    if (result != 0) {
        Log_Debug("ERROR: Could not set LED output value: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_LedTimer_SetLedState1;
        return;
    }
}

/// <summary>
///     Button timer event:  Check the status of the buttons.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }
    CheckButtonA();
    CheckButtonB();
}

/// <summary>
///     If the button has just been pressed, intentionally have the app crash by calling another
///     function. If DeferencingNull() is not called, the button press would switch the color of the
///     blinking LED. The button has GPIO_Value_Low when pressed and GPIO_Value_High when released.
/// </summary>
static void CheckButtonA(void)
{
    if (IsButtonPressed(ledBlinkButton1GpioFd, &button1State)) {
        DeferenceNull();
        if (button1State == GPIO_Value_Low) {
            // close the LEDs
            int result = GPIO_SetValue(
                buttonToggle ? blinkingLedBlueGpioFd : blinkingLedGreenGpioFd, GPIO_Value_High);
            if (result != 0) {
                Log_Debug("ERROR: Could not set LED output value: %s (%d).\n", strerror(errno),
                          errno);
                exitCode = ExitCode_ButtonTimer_SetLedState;
                return;
            }
            buttonToggle = !buttonToggle;
        }
    }
}

/// <summary>
///     Handle button timer event: if the button is pressed, intentionally exit the app.
/// </summary>
static void CheckButtonB(void)
{
    if (IsButtonPressed(ledBlinkButton2GpioFd, &button2State)) {
        if (button2State == GPIO_Value_Low) {
            exitCode = ExitCode_Exit_SuccessfulButtonBPress;
        }
    }
}

/// <summary>
///     Function that intentionally causes the application to crash.
///     The LED has GPIO_Value_Low when switched on and GPIO_Value_High when switched
///     off.
/// </summary>
static void DeferenceNull(void)
{
    GPIO_SetValue(blinkingLedBlueGpioFd, GPIO_Value_High);
    volatile int *pointer = NULL;
    *pointer;
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
        exitCode = ExitCode_ButtonTimer_GetButtonState;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Checks whether the network is ready.
///     If a fatal error occurs, sets exitCode and returns false.
/// </summary>
/// <returns>true if network is ready; false otherwise.</returns>
static bool IsNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_IsNetworkingReady_Failed;
        return false;
    }

    if (!isNetworkReady) {
        Log_Debug("Error: Make sure that network is ready before starting the tutorial.\n");
    }
    return isNetworkReady;
}

/// <summary>
///     <para>
///         This handler is called periodically when the program starts
///         to check whether network is ready. Once ready,
///         the timer is disarmed.  If a fatal error occurs, sets exitCode
///         to the appropriate value.
///     </para>
///     <para>
///         See <see cref="EventLoopTimerHandler" /> for more information
///         and a description of the argument.
///     </para>
/// </summary>
static void NetworkReadyCheckTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_NetworkReadyCheckHandler_Consume;
        return;
    }

    bool networkReady = IsNetworkReady();
    if (networkReady) {
        DisarmEventLoopTimer(timer);
        Log_Debug("INFO: Network is ready\n");
    }
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
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    ledBlinkButton1GpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (ledBlinkButton1GpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button1;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    ledBlinkButton2GpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (ledBlinkButton2GpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2;
    }

    // Set up a timer to poll the buttons
    struct timespec buttonPressCheckPeriod1Ms = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler,
                                                   &buttonPressCheckPeriod1Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // Open SAMPLE_RGBLED_BLUE GPIO, set as output with value GPIO_Value_High (off), and set up a
    // timer to blink it
    Log_Debug("Opening SAMPLE_RGBLED_BLUE as output.\n");
    blinkingLedBlueGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blinkingLedBlueGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_BLUE GPIO: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_LedBlue;
    }
    struct timespec blinkPeriod = {.tv_sec = 0, .tv_nsec = 500000000};
    blinkTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &BlinkingLedTimerEventHandler, &blinkPeriod);
    if (blinkTimer == NULL) {
        return ExitCode_Init_LedBlinkTimer;
    }

    // Open SAMPLE_RGBLED_GREEN GPIO, set as output with value GPIO_Value_High (off), and set up a
    // timer to blink it
    Log_Debug("Opening SAMPLE_RGBLED_GREEN as output.\n");
    blinkingLedGreenGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blinkingLedGreenGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_GREEN GPIO: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_LedGreen;
    }

    // Check is network is ready every second.
    static const struct timespec oneSecond = {.tv_sec = 1, .tv_nsec = 0};
    networkReadyCheckTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &NetworkReadyCheckTimerEventHandler, &oneSecond);
    if (networkReadyCheckTimer == NULL) {
        return ExitCode_Init_NetworkReadyCheckTimer;
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
    // Leave the LED off
    if (blinkingLedBlueGpioFd != -1) {
        GPIO_SetValue(blinkingLedBlueGpioFd, GPIO_Value_High);
    }
    if (blinkingLedGreenGpioFd != -1) {
        GPIO_SetValue(blinkingLedGreenGpioFd, GPIO_Value_High);
    }

    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(blinkTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(blinkingLedBlueGpioFd, "BlinkingLedBlueGpio");
    CloseFdAndPrintError(blinkingLedGreenGpioFd, "BlinkingLedGreenGpio");
    CloseFdAndPrintError(ledBlinkButton1GpioFd, "LedBlinkButton1Gpio");
    CloseFdAndPrintError(ledBlinkButton2GpioFd, "LedBlinkButton2Gpio");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Error Reporting application starting.\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    if (exitCode == ExitCode_Exit_SuccessfulButtonBPress) {
        exitCode = ExitCode_Success;
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}