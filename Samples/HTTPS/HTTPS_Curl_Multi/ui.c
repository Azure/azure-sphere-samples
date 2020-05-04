/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/networking.h>

#include "epoll_timerfd_utilities.h"
#include "log_utils.h"

// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

#include "web_client.h"
#include "ui.h"

// File descriptors - initialized to invalid value
static int blinkingLedGpioFd = -1;
static int blinkingLedTimerFd = -1;
static int triggerDownloadButtonGpioFd = -1;
static int buttonPollTimerFd = -1;
static int epollFd = -1;

// Initial status of LED
static bool ledState = GPIO_Value_High;
// Initial status of SAMPLE_BUTTON_1
static GPIO_Value_Type buttonState = GPIO_Value_High;

/// <summary>
///     Handle button timer event: if the button is pressed, an download is started if not alrady in
///     progress.
/// </summary>
static void ButtonPollTimerEventHandler(EventData *userData)
{
    if (ConsumeTimerFdEvent(userData->fd) != 0) {
        LogErrno("ERROR: cannot consume the timerfd event");
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

            //  Check whether the network is up before starting an cURL based web download.
            bool isNetworkingReady = false;
            if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
                Log_Debug("WARNING: Not starting the download because network is not up.\n");
                return;
            }

            if (WebClient_StartTransfers()) {
                Log_Debug("ERROR: error starting the downloads.\n");
            }
        }
        buttonState = newButtonState;
    }
}

/// The context of the timerfd for polling SAMPLE_BUTTON_1.
static EventData buttonPollTimerEventData = {.eventHandler = &ButtonPollTimerEventHandler};

/// <summary>
///     Blink SAMPLE_LED.
/// </summary>
static void BlinkingLedTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(eventData->fd) != 0) {
        LogErrno("ERROR: cannot consume the timerfd event");
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

/// The context of the timerfd for blinking SAMPLE_LED.
static EventData blinkingLedTimerEventData = {.eventHandler = &BlinkingLedTimerEventHandler};

ExitCode Ui_Init(int epollFdInstance)
{
    epollFd = epollFdInstance;

    // Open LED GPIO, set as output with value GPIO_Value_High (off), and set up a timer to poll it.
    Log_Debug("Opening SAMPLE_LED\n");
    blinkingLedGpioFd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blinkingLedGpioFd == -1) {
        LogErrno("ERROR: Could not open LED GPIO");
        return ExitCode_UiInit_SampleLed;
    }

    static const struct timespec halfSecondBlinkInterval = {.tv_sec = 0,
                                                            .tv_nsec = 500 * 1000 * 1000};
    blinkingLedTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &halfSecondBlinkInterval,
                                                    &blinkingLedTimerEventData, EPOLLIN);
    if (blinkingLedTimerFd == -1) {
        return ExitCode_UiInit_BlinkLed;
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
    buttonPollTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod,
                                                   &buttonPollTimerEventData, EPOLLIN);
    if (buttonPollTimerFd == -1) {
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
    CloseFdAndLogOnError(buttonPollTimerFd, "ButtonPollTimer");
    CloseFdAndLogOnError(blinkingLedTimerFd, "BlinkingLedTimer");
    CloseFdAndLogOnError(blinkingLedGpioFd, "BlinkingLedGpio");
}
