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
#include "mt3620_rdb.h"
#include "web_client.h"
#include "ui.h"

// File descriptors - initialized to invalid value
static int gpioLed1Fd = -1;
static int gpioLed1TimerFd = -1;
static int gpioButtonAFd = -1;
static int gpioButtonATimerFd = -1;
static int epollFd = -1;

// Initial status of LED1
static bool led1State = GPIO_Value_High;
// Initial status of button A
static GPIO_Value_Type buttonState = GPIO_Value_High;

/// The context of the timerfd for blinking LED1.
static event_data_t led1TimerEventData = {};

/// The context of the timerfd for pollling button A.
static event_data_t buttonATimerEventData = {};

/// <summary>
///     Handle button timer event: if the button is pressed, an download is started if not alrady in
///     progress.
/// </summary>
static void ButtonATimerEventHandler(event_data_t *userData)
{
    if (ConsumeTimerFdEvent(userData->fd) != 0) {
        LogErrno("ERROR: cannot consume the timerfd event");
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(gpioButtonAFd, &newButtonState);
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
            if ((Networking_IsNetworkingReady(&isNetworkingReady) < 0) || !isNetworkingReady) {
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

/// <summary>
///     Blink LED1.
/// </summary>
static void LedTimerEventHandler(event_data_t *eventData)
{
    if (ConsumeTimerFdEvent(eventData->fd) != 0) {
        Log_Debug("ERROR: cannot consume the timerfd event.\n");
        return;
    }

    // Blink the LED1 periodically.
    // The LED is active-low so GPIO_Value_Low is on and GPIO_Value_High is off
    led1State = (led1State == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);
    int result = GPIO_SetValue(gpioLed1Fd, led1State);
    if (result != 0) {
        LogErrno("ERROR: Could not set LED output value");
    }
}

int Ui_Init(int epollFdInstance)
{
    epollFd = epollFdInstance;

    // Open LED GPIO, set as output with value GPIO_Value_High (off), and set up a timer to poll it.
    Log_Debug("Opening MT3620_RDB_LED1_RED\n");
    gpioLed1Fd = GPIO_OpenAsOutput(MT3620_RDB_LED1_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (gpioLed1Fd < 0) {
        LogErrno("ERROR: Could not open LED GPIO");
        return -1;
    }

    static const struct timespec halfSecondBlinkInterval = {0, 500000000};
    gpioLed1TimerFd = CreateTimerFd(&halfSecondBlinkInterval);
    if (gpioLed1TimerFd < 0) {
        return -1;
    }
    led1TimerEventData.eventHandler = LedTimerEventHandler, led1TimerEventData.fd = gpioLed1TimerFd;
    if (RegisterEventHandlerToEpoll(epollFd, gpioLed1TimerFd, &led1TimerEventData, EPOLLIN) != 0) {
        return -1;
    }

    // Open button A GPIO as input, and set up a timer to poll it.
    Log_Debug("Opening MT3620_RDB_BUTTON_A as input.\n");
    gpioButtonAFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (gpioButtonAFd < 0) {
        LogErrno("ERROR: Could not open button GPIO");
        return -1;
    }
    // Check whether button A is pressed periodically.
    struct timespec buttonAPressCheckPeriod = {0, 100000000};
    gpioButtonATimerFd = CreateTimerFd(&buttonAPressCheckPeriod);
    if (gpioButtonATimerFd < 0) {
        return -1;
    }
    buttonATimerEventData.eventHandler = ButtonATimerEventHandler;
    buttonATimerEventData.fd = gpioButtonATimerFd;

    if (RegisterEventHandlerToEpoll(epollFd, gpioButtonATimerFd, &buttonATimerEventData, EPOLLIN) !=
        0) {
        return -1;
    }

    return 0;
}

void Ui_Fini(void)
{
    // Leave the LED off
    if (gpioLed1Fd >= 0) {
        GPIO_SetValue(gpioLed1Fd, GPIO_Value_High);
    }

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndLogOnError(gpioButtonAFd, "gpioButtonAFd");
    CloseFdAndLogOnError(gpioButtonATimerFd, "gpioButtonATimerFd");
    CloseFdAndLogOnError(gpioLed1TimerFd, "gpioLed1TimerFd");
    CloseFdAndLogOnError(gpioLed1Fd, "gpioLed1Fd");
}
