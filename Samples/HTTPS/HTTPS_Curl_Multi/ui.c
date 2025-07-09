/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/networking.h>

#include "eventloop_timer_utilities.h"
#include "log_utils.h"

// This #include imports the sample_appliance abstraction from that hardware definition.
#include <hw/sample_appliance.h>

#include "web_client.h"
#include "ui.h"

// File descriptors - initialized to invalid value
static int blinkingLedGpioFd = -1;
static EventLoopTimer *blinkingLedTimer = NULL;
static int triggerDownloadButtonGpioFd = -1;
static EventLoopTimer *buttonPollTimer = NULL;
static EventLoop *eventLoop = NULL; // not owned

// Initial status of LED
static bool ledState = GPIO_Value_High;
// Initial status of SAMPLE_BUTTON_1
static GPIO_Value_Type buttonState = GPIO_Value_High;

static bool IsNetworkReady(void);

/// <summary>
///     Checks that the network is ready.
/// </summary>
static bool IsNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!isNetworkReady) {
        Log_Debug("WARNING: Not doing download because the network is not ready.\n");
    }
    return isNetworkReady;
}

/// <summary>
///     Handle button timer event: if the button is pressed, an download is started if not alrady in
///     progress.
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        LogErrno("ERROR: cannot consume the timer event");
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(triggerDownloadButtonGpioFd, &newButtonState);
    if (result != 0) {
        LogErrno("ERROR: Could not read button GPIO");
        return;
    }

    // If the button has just been pressed, start the web page downloads.
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {

            //  Check whether the network is ready before starting a cURL based
            //  web download.
            if (IsNetworkReady() == false) {
                return;
            }

            if (WebClient_StartTransfers() != 0) {
                Log_Debug("ERROR: Error starting downloads\n");
            } else {
                Log_Debug("\n -==- Starting downloads -==-\n");
            }
        }
        buttonState = newButtonState;
    }
}

/// <summary>
///     Blink SAMPLE_LED.
/// </summary>
static void BlinkingLedTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        LogErrno("ERROR: cannot consume the timer event");
        return;
    }

    // Blink the SAMPLE_LED periodically.
    // The SAMPLE_LED is active-low so GPIO_Value_Low is on and GPIO_Value_High is off
    ledState = (ledState == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);
    int result = GPIO_SetValue(blinkingLedGpioFd, ledState);
    if (result != 0) {
        LogErrno("ERROR: Could not set LED output value");
    }
}

ExitCode Ui_Init(EventLoop *eventLoopInstance)
{
    eventLoop = eventLoopInstance;

    // Open LED GPIO, set as output with value GPIO_Value_High (off), and set up a timer to poll it.
    Log_Debug("Opening SAMPLE_LED\n");
    blinkingLedGpioFd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blinkingLedGpioFd == -1) {
        LogErrno("ERROR: Could not open LED GPIO");
        return ExitCode_UiInit_SampleLed;
    }

    static const struct timespec halfSecondBlinkInterval = {.tv_sec = 0,
                                                            .tv_nsec = 500 * 1000 * 1000};
    blinkingLedTimer = CreateEventLoopPeriodicTimer(eventLoop, &BlinkingLedTimerEventHandler,
                                                    &halfSecondBlinkInterval);

    if (blinkingLedTimer == NULL) {
        return ExitCode_UiInit_BlinkTimer;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it.
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    triggerDownloadButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (triggerDownloadButtonGpioFd == -1) {
        LogErrno("ERROR: Could not open SAMPLE_BUTTON_1");
        return ExitCode_UiInit_Button;
    }
    // Periodically check whether SAMPLE_BUTTON_1 is pressed.
    struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_UiInit_ButtonPollTimer;
    }

    return ExitCode_Success;
}

void Ui_Fini(void)
{
    // Leave the LED off
    if (blinkingLedGpioFd >= 0) {
        GPIO_SetValue(blinkingLedGpioFd, GPIO_Value_High);
    }

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndLogOnError(triggerDownloadButtonGpioFd, "TriggerDownloadButtonGpio");
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(blinkingLedTimer);
    CloseFdAndLogOnError(blinkingLedGpioFd, "BlinkingLedGpio");
}
