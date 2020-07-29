/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This application forms part of the Wi-Fi setup and device control via BLE reference solution for
// Azure Sphere.
//
// It implements communication between an Azure Sphere MCU and the sibling application
// running on a Nordic nRF52 Bluetooth LE board, allowing Wi-Fi configuration and LED control on the
// Azure Sphere via Bluetooth LE.
//
// Pressing SAMPLE_BUTTON_1 briefly will start allowing new BLE bonds for 1 minute.
// Holding SAMPLE_BUTTON_1 will delete all BLE bonds.
// Pressing SAMPLE_BUTTON_2 briefly will toggle SAMPLE_DEVICE_STATUS_LED.
// Holding SAMPLE_BUTTON_2 will forget all stored Wi-Fi networks on Azure Sphere.
// SAMPLE_RGBLED will be illuminated to a color which indicates the BLE status:
//      Yellow  - Uninitialized;
//      Blue    - Advertising to bonded devices only;
//      Red     - Advertising to all devices;
//      Green   - Connected to a central device;
//      Magenta - Error
//
// It uses the API for the following Azure Sphere application libraries:
// - UART (serial port)
// - GPIO (digital inputs and outputs)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - wificonfig (configure Wi-Fi settings)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/uart.h>
#include <applibs/log.h>
#include <applibs/wificonfig.h>

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_DIRECTORY argument in the call to
// azsphere_target_hardware_definition to "HardwareDefinitions/avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
#include <hw/sample_appliance.h>

// This sample uses a single-thread event loop pattern, based on epoll and timerfd
#include "epoll_timerfd_utilities.h"

#include "message_protocol.h"
#include "blecontrol_message_protocol.h"
#include "wificonfig_message_protocol.h"
#include "devicecontrol_message_protocol.h"
#include "exitcode_wifible.h"

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
static GPIO_Value_Type deviceStatusLedGpioFd = GPIO_Value_High;

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
static ButtonState button1State = {.fd = -1, .isPressed = false, .isHeld = false};
static ButtonState button2State = {.fd = -1, .isPressed = false, .isHeld = false};

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
static volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
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
        // Illuminate SAMPLE_RGBLED yellow (green + red) to indicate BLE is in
        // the uninitialized state.
        GPIO_SetValue(bleAdvertiseToAllDevicesLedGpioFd, GPIO_Value_Low);
        GPIO_SetValue(bleConnectedLedGpioFd, GPIO_Value_Low);
    } else if (state == BleControlMessageProtocolState_Error) {
        // Illuminate SAMPLE_RGBLED magenta (blue + red) to indicate BLE is in
        // the error state.
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
    deviceStatusLedGpioFd = (isOn ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(deviceControlLedGpioFd, deviceStatusLedGpioFd);
}

/// <summary>
///     Get status for the Device Control LED.
/// </summary>
/// <returns>The status of Device Control LED.</returns>
static bool GetDeviceControlLedStatusHandler(void)
{
    return (deviceStatusLedGpioFd == GPIO_Value_Low);
}

/// <summary>
///     Handle button timer event and take defined actions as printed when the application started.
/// </summary>
/// <param name="eventData">Context data for handled event.</param>
static void ButtonTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(buttonTimerFd) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Take actions based on button events.
    ButtonEvent button1Event = GetButtonEvent(&button1State);
    if (button1Event == ButtonEvent_Error) {
        exitCode = ExitCode_ButtonTimer_GetEvent1;
        return;
    } else if (button1Event == ButtonEvent_Released) {
        // SAMPLE_BUTTON_1 has just been released without being held, start BLE advertising to all
        // devices.
        Log_Debug("INFO: SAMPLE_BUTTON_1 was pressed briefly, allowing new BLE bonds...\n");
        if (BleControlMessageProtocol_AllowNewBleBond(&bleAdvertiseToAllTimeoutPeriod) != 0) {
            Log_Debug("ERROR: Unable to allow new BLE bonds, check nRF52 is connected.\n");
        }
    } else if (button1Event == ButtonEvent_Held) {
        // When SAMPLE_BUTTON_1 is held, delete all bonded BLE devices.
        Log_Debug("INFO: SAMPLE_BUTTON_1 is held; deleting all BLE bonds...\n");
        if (BleControlMessageProtocol_DeleteAllBondedDevices() != 0) {
            Log_Debug("ERROR: Unable to delete all BLE bonds, check nRF52 is connected.\n");
        } else {
            Log_Debug("INFO: All BLE bonds are deleted successfully.\n");
        }
    }
    // No actions are defined for other events.

    // Take actions based on SAMPLE_BUTTON_2 events.
    ButtonEvent button2Event = GetButtonEvent(&button2State);
    if (button2Event == ButtonEvent_Error) {
        exitCode = ExitCode_ButtonTimer_GetEvent2;
        return;
    } else if (button2Event == ButtonEvent_Released) {
        Log_Debug(
            "INFO: SAMPLE_BUTTON_2 was pressed briefly; toggling SAMPLE_DEVICE_STATUS_LED.\n");
        deviceStatusLedGpioFd =
            (deviceStatusLedGpioFd == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);
        GPIO_SetValue(deviceControlLedGpioFd, deviceStatusLedGpioFd);
        DeviceControlMessageProtocol_NotifyLedStatusChange();
    } else if (button2Event == ButtonEvent_Held) {
        // Forget all stored Wi-Fi networks
        Log_Debug("INFO: SAMPLE_BUTTON_2 is held; forgetting all stored Wi-Fi networks...\n");
        if (WifiConfig_ForgetAllNetworks() != 0) {
            Log_Debug("ERROR: Unable to forget all stored Wi-Fi networks: %s (%d).\n",
                      strerror(errno), errno);
        } else {
            Log_Debug("INFO: All stored Wi-Fi networks are forgotten successfully.\n");
        }
    }
    // No actions are defined for other events.
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonsEventData = {.eventHandler = &ButtonTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    // Open the GPIO controlling the nRF52 reset pin, and keep it held in reset (low) until needed.
    bleDeviceResetPinGpioFd =
        GPIO_OpenAsOutput(SAMPLE_NRF52_RESET, GPIO_OutputMode_OpenDrain, GPIO_Value_Low);
    if (bleDeviceResetPinGpioFd == -1) {
        Log_Debug("ERROR: Could not open GPIO 5 as reset pin: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ResetPin;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd == -1) {
        return ExitCode_Init_Epoll;
    }

    // Open the UART and set up UART event handler.
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_RTSCTS;
    uartFd = UART_Open(SAMPLE_NRF52_UART, &uartConfig);
    if (uartFd == -1) {
        Log_Debug("ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Uart;
    }

    ExitCode localExitCode = MessageProtocol_Init(epollFd, uartFd);
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    BleControlMessageProtocol_Init(BleStateChangeHandler, epollFd);
    WifiConfigMessageProtocol_Init();
    DeviceControlMessageProtocol_Init(SetDeviceControlLedStatusHandler,
                                      GetDeviceControlLedStatusHandler);

    Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
    button1State.fd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (button1State.fd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button1;
    }

    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    button2State.fd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (button2State.fd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button2;
    }

    struct timespec buttonStatusCheckPeriod = {0, 1000000};
    buttonTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &buttonStatusCheckPeriod, &buttonsEventData, EPOLLIN);
    if (buttonTimerFd == -1) {
        return ExitCode_Init_ButtonTimer;
    }

    // Open SAMPLE_RGBLED_BLUE GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening SAMPLE_RGBLED_BLUE\n");
    bleAdvertiseToBondedDevicesLedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleAdvertiseToBondedDevicesLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_BLUE GPIO: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_BondedDevicesLed;
    }

    // Open SAMPLE_RGBLED_RED GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening SAMPLE_RGBLED_RED\n");
    bleAdvertiseToAllDevicesLedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleAdvertiseToAllDevicesLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_RED GPIO: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_AllDevicesLed;
    }

    // Open SAMPLE_RGBLED_GREEN GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening SAMPLE_RGBLED_GREEN\n");
    bleConnectedLedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (bleConnectedLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_RGBLED_GREEN GPIO: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_BleConnectedLed;
    }

    // Open SAMPLE_DEVICE_STATUS_LED GPIO and set as output with value GPIO_Value_High (off).
    Log_Debug("Opening SAMPLE_DEVICE_STATUS_LED\n");
    deviceStatusLedGpioFd = GPIO_Value_High;
    deviceControlLedGpioFd = GPIO_OpenAsOutput(SAMPLE_DEVICE_STATUS_LED, GPIO_OutputMode_PushPull,
                                               deviceStatusLedGpioFd);
    if (deviceControlLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_DEVICE_STATUS_LED GPIO: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_Init_DeviceControlLed;
    }

    UpdateBleLedStatus(BleControlMessageProtocolState_Uninitialized);

    // Initialization completed, start the nRF52 application.
    GPIO_SetValue(bleDeviceResetPinGpioFd, GPIO_Value_High);

    return ExitCode_Success;
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
    CloseFdAndPrintError(button1State.fd, "Button1");
    CloseFdAndPrintError(button2State.fd, "Button2");
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
        "\tPress SAMPLE_BUTTON_1  - Start allowing new BLE bonds for 1 minute\n"
        "\tHold SAMPLE_BUTTON_1   - Delete all BLE bonds\n"
        "\tPress SAMPLE_BUTTON_2  - Toggle SAMPLE_DEVICE_STATUS_LED\n"
        "\tHold SAMPLE_BUTTON_2   - Forget all stored Wi-Fi networks on Azure Sphere device\n\n"
        "SAMPLE_RGBLED's color indicates BLE status for the attached nRF52 board:\n"
        "\tYellow  - Uninitialized\n"
        "\tBlue    - Advertising to bonded devices only\n"
        "\tRed     - Advertising to all devices\n"
        "\tGreen   - Connected to a central device\n"
        "\tMagenta - Error\n\n");

    exitCode = InitPeripheralsAndHandlers();

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            exitCode = ExitCode_Main_EventCall;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("INFO: Application exiting\n");
    return exitCode;
}