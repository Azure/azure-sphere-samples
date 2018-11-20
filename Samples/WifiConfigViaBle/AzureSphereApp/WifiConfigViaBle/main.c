/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "message_protocol.h"
#include "blecontrol_message_protocol.h"
#include "wificonfig_message_protocol.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/wificonfig.h>

#include "mt3620_rdb.h"

// This application forms part of the Bluetooth LE sample for Azure Sphere.
//
// It implements communication between an MT3620 development board and the sibling application
// running on a Nordic nRF52 Bluetooth LE board, allowing configuration of Wi-Fi on the MT3620
// via Bluetooth LE.
//
// Pressing button A will reset the nRF52 board to restart its application.
// Pressing button B will forget all stored Wi-Fi networks on MT3620.
// LED2 will be illuminated blue if the nRF52 board is advertising over BLE.
//
// It uses the API for the following Azure Sphere application libraries:
// - UART (serial port)
// - GPIO (digital input for button)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - wificonfig (configure Wi-Fi settings)

#define BLE_ACTIVE_DURATION 300u

// File descriptors - initialized to invalid value
static int gpioButtonAFd = -1;
static int gpioButtonBFd = -1;
static int gpioButtonTimerFd = -1;
static int gpioBleAdvertisingLedFd = -1;
static int epollFd = -1;
static int gpioBleDeviceResetPinFd = -1;
static int bleActiveDurationTimerFd = -1;

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
///     Stop BLE device.
/// </summary>
static void stopBleDevice(void)
{
    Log_Debug("INFO: Stopping nRF52.\n");
    GPIO_SetValue(gpioBleDeviceResetPinFd, GPIO_Value_Low);

	// Ensure LED is off, indicating that the BLE device is not (no longer) advertising.
	GPIO_SetValue(gpioBleAdvertisingLedFd, GPIO_Value_High);
}

/// <summary>
///     Start BLE device.
/// </summary>
static void startBleDevice(void)
{
    Log_Debug("INFO: Starting nRF52.\n");
    GPIO_SetValue(gpioBleDeviceResetPinFd, GPIO_Value_High);

	// Start timer, after which BLE device will be stopped again.
    struct timespec bleActiveDurationPeriod = {BLE_ACTIVE_DURATION, 0};
    SetTimerFdToSingleExpiry(bleActiveDurationTimerFd, &bleActiveDurationPeriod);
}

/// <summary>
///     Handle notification that BLE device has started advertising.
/// </summary>
static void BleAdvertisingStartedHandler(void)
{
    Log_Debug("INFO: BLE device is now advertising.\n");
    GPIO_SetValue(gpioBleAdvertisingLedFd, GPIO_Value_Low);
}

/// <summary>
///     Handle BLE active duration: stop the nRF52.
/// </summary>
/// <param name="eventData">Context data for handled event.</param>
static void bleActiveDurationEventHandler(event_data_t *eventData)
{
    if (ConsumeTimerFdEvent(bleActiveDurationTimerFd) != 0) {
        return;
    }

    Log_Debug("INFO: BLE device active duration reached.\n");
    stopBleDevice();
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
///     Handle button timer event:
///     when the "Button A" is pressed, start BLE device advertising.
///     when the "Button B" is pressed, forget all stored Wi-Fi networks.
/// </summary>
/// <param name="eventData">Context data for handled event.</param>
static void ButtonTimerEventHandler(event_data_t *eventData)
{
    if (ConsumeTimerFdEvent(gpioButtonTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // Check for button A press
    static GPIO_Value_Type newButtonAState;
    if (IsButtonPressed(gpioButtonAFd, &newButtonAState)) {
		// Restart BLE device.
		stopBleDevice();
        startBleDevice();
    }

    // Check for button B press
    static GPIO_Value_Type newButtonBState;
    if (IsButtonPressed(gpioButtonBFd, &newButtonBState)) {
        // Forget all stored Wi-Fi networks
        int wifiResult = WifiConfig_ForgetAllNetworks();
        if (wifiResult != 0) {
            Log_Debug("ERROR: Fail to forget all stored Wi-Fi networks.\n");
        } else {
            Log_Debug("INFO: All stored Wi-Fi networks are forgotten successfully.\n");
        }
    }
}

// event handler data structures. Only the event handler field needs to be populated.
static event_data_t buttonsEventData = {.eventHandler = &ButtonTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure.</returns>
static int InitPeripheralsAndHandlers(void)
{
    // Open the GPIO controlling the nRF52 reset pin, and keep it held in reset (low) until needed.
    gpioBleDeviceResetPinFd = GPIO_OpenAsOutput(MT3620_GPIO5, GPIO_OutputMode_OpenDrain, GPIO_Value_Low);
    if (gpioBleDeviceResetPinFd < 0) {
        Log_Debug("ERROR: Could not open GPIO 5 as reset pin: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }
    if (MessageProtocol_Init(epollFd) < 0) {
        return -1;
    }

    BleControlMessageProtocol_Init(BleAdvertisingStartedHandler);
    WifiConfigMessageProtocol_Init();

    Log_Debug("Opening MT3620_RDB_BUTTON_A as input\n");
    gpioButtonAFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (gpioButtonAFd < 0) {
        Log_Debug("ERROR: Could not open button A GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    Log_Debug("Opening MT3620_RDB_BUTTON_B as input.\n");
    gpioButtonBFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_B);
    if (gpioButtonBFd < 0) {
        Log_Debug("ERROR: Could not open button B GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    struct timespec buttonPressCheckPeriod = {0, 1000000};
    gpioButtonTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonsEventData, EPOLLIN);
    if (gpioButtonTimerFd < 0) {
        return -1;
    }

    // Open LED GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening MT3620_RDB_LED2_BLUE\n");
    gpioBleAdvertisingLedFd =
		GPIO_OpenAsOutput(MT3620_RDB_LED2_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (gpioBleAdvertisingLedFd < 0) {
        Log_Debug("ERROR: Could not open LED GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

	// Set up BLE active duration timer, for later use.
    struct timespec disabled = {0, 0};
    static event_data_t bleActiveDurationEventData = {.eventHandler =
                                                          &bleActiveDurationEventHandler};
    bleActiveDurationTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &disabled, &bleActiveDurationEventData, EPOLLIN);
    if (bleActiveDurationTimerFd < 0) {
        return -1;
    }
 
    // Leave the nRF52 stopped (reset pin held), awaiting explicit user action to start it.
    Log_Debug("INFO: Press button A to start the nRF52, and begin BLE advertising.\n");

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    // Leave the LED off
    if (gpioBleAdvertisingLedFd >= 0) {
        GPIO_SetValue(gpioBleAdvertisingLedFd, GPIO_Value_High);
    }

    Log_Debug("Closing file descriptors\n");
    CloseFdAndPrintError(gpioButtonTimerFd, "ButtonTimer");
    CloseFdAndPrintError(gpioButtonAFd, "ButtonA");
    CloseFdAndPrintError(gpioButtonBFd, "ButtonB");
    CloseFdAndPrintError(gpioBleDeviceResetPinFd, "BleDeviceResetPin");
    CloseFdAndPrintError(gpioBleAdvertisingLedFd, "BleAdvertisingLed");
    CloseFdAndPrintError(bleActiveDurationTimerFd, "BleActiveDurationTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
    WifiConfigMessageProtocol_Cleanup();
    BleControlMessageProtocol_Cleanup();
    MessageProtocol_Cleanup();
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("INFO: BLE Wi-Fi application starting.\n");
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
    Log_Debug("INFO: Application exiting\n");
    return 0;
}