/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "message_protocol.h"
#include "blecontrol_message_protocol.h"
#include "wificonfig_message_protocol.h"
#include "devicecontrol_message_protocol.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/wificonfig.h>

#include "mt3620_rdb.h"

// This application forms part of the Wi-Fi setup and device control via BLE reference solution for
// Azure Sphere.
//
// It implements communication between an MT3620 development board and the sibling application
// running on a Nordic nRF52 Bluetooth LE board, allowing Wi-Fi configuration and LED control on the
// MT3620 via Bluetooth LE.
//
// Pressing button A briefly will start allowing new BLE bonds for 1 minute.
// Holding button A will delete all BLE bonds.
// Pressing button B briefly will toggle LED3.
// Holding button B will forget all stored Wi-Fi networks on MT3620.
// LED2 will be illuminated to a color which indicates the BLE status:
//      Yellow  - Uninitialized;
//      Blue    - Advertising to bonded devices only;
//      Red     - Advertising to all devices;
//      Green   - Connected to a central device;
//      Magenta - Error
//
// It uses the API for the following Azure Sphere application libraries:
// - UART (serial port)
// - GPIO (digital input for button)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - wificonfig (configure Wi-Fi settings)

// File descriptors - initialized to invalid value
static int buttonTimerFd = -1;
static int bleAdvertiseToBondedDevicesLedGpioFd = -1;
static int bleAdvertiseToAllDevicesLedGpioFd = -1;
static int bleConnectedLedGpioFd = -1;
static int deviceControlLedGpioFd = -1;
static int epollFd = -1;
static int uartFd = -1;
static int bleDeviceResetPinGpioFd = -1;
static struct timespec bleAdvertiseToAllTimeoutPeriod = {60u, 0};
static GPIO_Value_Type deviceControlLedState = GPIO_Value_High;

/// <summary>
///     Button events.
/// </summary>
typedef enum {
    /// <summary>The event when failing to get button state.</summary>
    ButtonEvent_Error = -1,
    /// <summary>No button event has occurred.</summary>
    ButtonEvent_None = 0,
    /// <summary>The event when button is pressed.</summary>
    ButtonEvent_Pressed,
    /// <summary>The event when button is released.</summary>
    ButtonEvent_Released,
    /// <summary>The event when button is being held.</summary>
    ButtonEvent_Held,
    /// <summary>The event when button is released after being held.</summary>
    ButtonEvent_ReleasedAfterHeld
} ButtonEvent;

/// <summary>
///     Data structure for the button state.
/// </summary>
typedef struct {
    /// <summary>File descriptor for the button.</summary>
    int fd;
    /// <summary>Whether the button is currently pressed.</summary>
    bool isPressed;
    /// <summary>Whether the button is currently held.</summary>
    bool isHeld;
    /// <summary>The time stamp when the button-press happened.</summary>
    struct timespec pressedTime;
} ButtonState;

// Button-related variables
static const time_t buttonHeldThresholdTimeInSeconds = 3L;
static ButtonState buttonAState = {.fd = -1, .isPressed = false, .isHeld = false};
static ButtonState buttonBState = {.fd = -1, .isPressed = false, .isHeld = false};

ButtonEvent GetButtonEvent(ButtonState *state)
{
    ButtonEvent event = ButtonEvent_None;
    GPIO_Value_Type newInputValue;
    int result = GPIO_GetValue(state->fd, &newInputValue);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        return ButtonEvent_Error;
    }
    if (state->isPressed) {
        if (newInputValue == GPIO_Value_High) {
            // Button has just been released, so set event based on whether button has been held and
            // then reset flags.
            event = state->isHeld ? ButtonEvent_ReleasedAfterHeld : ButtonEvent_Released;
            state->isHeld = false;
            state->isPressed = false;
        }
        if (newInputValue == GPIO_Value_Low && !state->isHeld) {
            // Button has been pressed and hasn't been released yet. As it hasn't been classified as
            // held, compare the elapsed time to determine whether the button has been held long
            // enough to be regarded as 'Held'.
            struct timespec currentTime;
            clock_gettime(CLOCK_REALTIME, &currentTime);
            long elapsedSeconds = currentTime.tv_sec - state->pressedTime.tv_sec;
            state->isHeld = (elapsedSeconds > buttonHeldThresholdTimeInSeconds ||
                             (elapsedSeconds == buttonHeldThresholdTimeInSeconds &&
                              currentTime.tv_nsec >= state->pressedTime.tv_nsec));
            if (state->isHeld) {
                event = ButtonEvent_Held;
            }
        }
    } else if (newInputValue == GPIO_Value_Low) {
        // Button has just been pressed, set isPressed flag and mark current time.
        state->isPressed = true;
        clock_gettime(CLOCK_REALTIME, &state->pressedTime);
        event = ButtonEvent_Pressed;
    }
    return event;
}

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

static void UpdateBleLedStatus(BleControlMessageProtocolState state)
{
    GPIO_SetValue(bleAdvertiseToBondedDevicesLedGpioFd,
                  (state == BleControlMessageProtocolState_AdvertiseToBondedDevices)
                      ? GPIO_Value_Low
                      : GPIO_Value_High);
    GPIO_SetValue(bleAdvertiseToAllDevicesLedGpioFd,
                  (state == BleControlMessageProtocolState_AdvertisingToAllDevices)
                      ? GPIO_Value_Low
                      : GPIO_Value_High);
    GPIO_SetValue(bleConnectedLedGpioFd, (state == BleControlMessageProtocolState_DeviceConnected)
                                             ? GPIO_Value_Low
                                             : GPIO_Value_High);
    if (state == BleControlMessageProtocolState_Uninitialized) {
        // Illuminate LED to Yellow (Green + Red) to indicate BLE is in the uninitialized state.
        GPIO_SetValue(bleAdvertiseToAllDevicesLedGpioFd, GPIO_Value_Low);
        GPIO_SetValue(bleConnectedLedGpioFd, GPIO_Value_Low);
    } else if (state == BleControlMessageProtocolState_Error) {
        // Illuminate LED to Magenta (Blue + Red) to indicate BLE is in the error state.
        GPIO_SetValue(bleAdvertiseToBondedDevicesLedGpioFd, GPIO_Value_Low);
        GPIO_SetValue(bleAdvertiseToAllDevicesLedGpioFd, GPIO_Value_Low);
    }
}

/// <summary>
///     Handle notification of state change generated by the attached BLE device.
/// </summary>
/// <param name="state">The new state of attached BLE device.</param>
static void BleStateChangeHandler(BleControlMessageProtocolState state)
{
    UpdateBleLedStatus(state);
    switch (state) {
    case BleControlMessageProtocolState_Error:
        Log_Debug("INFO: BLE device is in an error state, resetting it...\n");
        GPIO_SetValue(bleDeviceResetPinGpioFd, GPIO_Value_Low);
        GPIO_SetValue(bleDeviceResetPinGpioFd, GPIO_Value_High);
        break;
    case BleControlMessageProtocolState_AdvertiseToBondedDevices:
        Log_Debug("INFO: BLE device is advertising to bonded devices only.\n");
        break;
    case BleControlMessageProtocolState_AdvertisingToAllDevices:
        Log_Debug("INFO: BLE device is advertising to all devices.\n");
        break;
    case BleControlMessageProtocolState_DeviceConnected:
        Log_Debug("INFO: BLE device is now connected to a central device.\n");
        break;
    case BleControlMessageProtocolState_Uninitialized:
        Log_Debug("INFO: BLE device is now being initialized.\n");
        break;
    default:
        Log_Debug("ERROR: Unsupported BLE state: %d.\n", state);
        break;
    }
}

/// <summary>
///     Set the Device Control LED's status.
/// </summary>
/// <param name="state">The LED status to be set.</param>
static void SetDeviceControlLedStatusHandler(bool isOn)
{
    deviceControlLedState = (isOn ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(deviceControlLedGpioFd, deviceControlLedState);
}

/// <summary>
///     Get status for the Device Control LED.
/// </summary>
/// <returns>The status of Device Control LED.</returns>
static bool GetDeviceControlLedStatusHandler(void)
{
    return (deviceControlLedState == GPIO_Value_Low);
}

/// <summary>
///     Handle button timer event and take defined actions as printed when the application started.
/// </summary>
/// <param name="eventData">Context data for handled event.</param>
static void ButtonTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(buttonTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // Take actions based on button A events.
    ButtonEvent buttonAEvent = GetButtonEvent(&buttonAState);
    switch (buttonAEvent) {
    case ButtonEvent_Error:
        terminationRequired = true;
        return;
    case ButtonEvent_Released:
        // Button A has just been released without being held, start BLE advertising to all devices.
        Log_Debug("INFO: 'Button A' was pressed briefly, allowing new BLE bonds...\n");
        if (BleControlMessageProtocol_AllowNewBleBond(&bleAdvertiseToAllTimeoutPeriod) != 0) {
            Log_Debug("ERROR: Unable to allow new BLE bonds, check nRF52 is connected.\n");
        }
        break;
    case ButtonEvent_Held:
        // When Button A is held, delete all bonded BLE devices.
        Log_Debug("INFO: 'Button A' is held, deleting all BLE bonds...\n");
        if (BleControlMessageProtocol_DeleteAllBondedDevices() != 0) {
            Log_Debug("ERROR: Unable to delete all BLE bonds, check nRF52 is connected.\n");
        } else {
            Log_Debug("INFO: All BLE bonds are deleted successfully.\n");
        }
        break;
    default:
        // No actions are defined for other events.
        break;
    }

    // Take actions based on button B events.
    ButtonEvent buttonBEvent = GetButtonEvent(&buttonBState);
    switch (buttonBEvent) {
    case ButtonEvent_Error:
        terminationRequired = true;
        return;
    case ButtonEvent_Released:
        Log_Debug("INFO: 'Button B' was pressed briefly, toggle LED3.\n");
        deviceControlLedState =
            (deviceControlLedState == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);
        GPIO_SetValue(deviceControlLedGpioFd, deviceControlLedState);
        DeviceControlMessageProtocol_NotifyLedStatusChange();
        break;
    case ButtonEvent_Held:
        // Forget all stored Wi-Fi networks
        Log_Debug("INFO: 'Button B' is held, forgetting all stored Wi-Fi networks...\n");
        if (WifiConfig_ForgetAllNetworks() != 0) {
            Log_Debug("ERROR: Unable to forget all stored Wi-Fi networks: %s (%d).\n",
                      strerror(errno), errno);
        } else {
            Log_Debug("INFO: All stored Wi-Fi networks are forgotten successfully.\n");
        }
        break;
    default:
        // No actions are defined for other events.
        break;
    }
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonsEventData = {.eventHandler = &ButtonTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure.</returns>
static int InitPeripheralsAndHandlers(void)
{
    // Open the GPIO controlling the nRF52 reset pin, and keep it held in reset (low) until needed.
    bleDeviceResetPinGpioFd =
        GPIO_OpenAsOutput(MT3620_GPIO5, GPIO_OutputMode_OpenDrain, GPIO_Value_Low);
    if (bleDeviceResetPinGpioFd < 0) {
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

    // Open the UART and set up UART event handler.
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_RTSCTS;
    uartFd = UART_Open(MT3620_RDB_HEADER2_ISU0_UART, &uartConfig);
    if (uartFd < 0) {
        Log_Debug("ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    if (MessageProtocol_Init(epollFd, uartFd) < 0) {
        return -1;
    }

    BleControlMessageProtocol_Init(BleStateChangeHandler, epollFd);
    WifiConfigMessageProtocol_Init();
    DeviceControlMessageProtocol_Init(SetDeviceControlLedStatusHandler,
                                      GetDeviceControlLedStatusHandler);

    Log_Debug("Opening MT3620_RDB_BUTTON_A as input\n");
    buttonAState.fd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (buttonAState.fd < 0) {
        Log_Debug("ERROR: Could not open button A GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    Log_Debug("Opening MT3620_RDB_BUTTON_B as input.\n");
    buttonBState.fd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_B);
    if (buttonBState.fd < 0) {
        Log_Debug("ERROR: Could not open button B GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    struct timespec buttonStatusCheckPeriod = {0, 1000000};
    buttonTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &buttonStatusCheckPeriod, &buttonsEventData, EPOLLIN);
    if (buttonTimerFd < 0) {
        return -1;
    }

    // Open blue LED2 GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening MT3620_RDB_LED2_BLUE\n");
    bleAdvertiseToBondedDevicesLedGpioFd =
        GPIO_OpenAsOutput(MT3620_RDB_LED2_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleAdvertiseToBondedDevicesLedGpioFd < 0) {
        Log_Debug("ERROR: Could not open blue LED2 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open red LED2 GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening MT3620_RDB_LED2_RED\n");
    bleAdvertiseToAllDevicesLedGpioFd =
        GPIO_OpenAsOutput(MT3620_RDB_LED2_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleAdvertiseToAllDevicesLedGpioFd < 0) {
        Log_Debug("ERROR: Could not open red LED2 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open green LED2 GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening MT3620_RDB_LED2_GREEN\n");
    bleConnectedLedGpioFd =
        GPIO_OpenAsOutput(MT3620_RDB_LED2_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleConnectedLedGpioFd < 0) {
        Log_Debug("ERROR: Could not open green LED2 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // Open green LED3 GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening MT3620_RDB_LED3_GREEN\n");
    deviceControlLedState = GPIO_Value_High;
    deviceControlLedGpioFd =
        GPIO_OpenAsOutput(MT3620_RDB_LED3_GREEN, GPIO_OutputMode_PushPull, deviceControlLedState);
    if (deviceControlLedGpioFd < 0) {
        Log_Debug("ERROR: Could not open green LED3 GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    UpdateBleLedStatus(BleControlMessageProtocolState_Uninitialized);

    // Initialization completed, start the nRF52 application.
    GPIO_SetValue(bleDeviceResetPinGpioFd, GPIO_Value_High);

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    // Leave the LED off
    if (bleAdvertiseToBondedDevicesLedGpioFd >= 0) {
        GPIO_SetValue(bleAdvertiseToBondedDevicesLedGpioFd, GPIO_Value_High);
    }
    if (bleAdvertiseToAllDevicesLedGpioFd >= 0) {
        GPIO_SetValue(bleAdvertiseToAllDevicesLedGpioFd, GPIO_Value_High);
    }
    if (bleConnectedLedGpioFd >= 0) {
        GPIO_SetValue(bleConnectedLedGpioFd, GPIO_Value_High);
    }

    Log_Debug("Closing file descriptors\n");
    CloseFdAndPrintError(buttonTimerFd, "ButtonTimer");
    CloseFdAndPrintError(buttonAState.fd, "ButtonA");
    CloseFdAndPrintError(buttonBState.fd, "ButtonB");
    CloseFdAndPrintError(bleDeviceResetPinGpioFd, "BleDeviceResetPin");
    CloseFdAndPrintError(bleAdvertiseToBondedDevicesLedGpioFd, "BleAdvertiseToBondedDevicesLed");
    CloseFdAndPrintError(bleAdvertiseToAllDevicesLedGpioFd, "BleAdvertiseToAllDevicesLed");
    CloseFdAndPrintError(bleConnectedLedGpioFd, "BleConnectedLed");
    CloseFdAndPrintError(epollFd, "Epoll");
    CloseFdAndPrintError(uartFd, "Uart");
    DeviceControlMessageProtocol_Cleanup();
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
    Log_Debug(
        "Available actions on the Azure Sphere device:\n"
        "\tPress button A  - Start allowing new BLE bonds for 1 minute\n"
        "\tHold button A   - Delete all BLE bonds\n"
        "\tPress button B  - Toggle LED3\n"
        "\tHold button B   - Forget all stored Wi-Fi networks on MT3620\n\n"
        "LED2's color indicates BLE status for the attached nRF52 board:\n"
        "\tYellow  - Uninitialized\n"
        "\tBlue    - Advertising to bonded devices only\n"
        "\tRed     - Advertising to all devices\n"
        "\tGreen   - Connected to a central device\n"
        "\tMagenta - Error\n\n");
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