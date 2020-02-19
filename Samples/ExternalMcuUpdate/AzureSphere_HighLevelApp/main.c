﻿/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/uart.h>
#include <applibs/gpio.h>
#include <applibs/log.h>

// By default, this sample's CMake build targets hardware that follows the MT3620
// Reference Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studios.
//
// To target different hardware, you'll need to update the CMake build. The necessary
// steps to do this vary depending on if you are building in Visual Studio, in Visual
// Studio Code or via the command line.
//
// See https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

#include "nordic/dfu_uart_protocol.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They they must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ButtonTimerHandler_Consume = 2,
    ExitCode_ButtonTimerHandler_GetValue = 3,

    ExitCode_Init_Reset = 4,
    ExitCode_Init_Epoll = 5,
    ExitCode_Init_Uart = 6,
    ExitCode_Init_DfuMode = 7,
    ExitCode_Init_Trigger = 8,
    ExitCode_Init_ButtonTimer = 9,

    ExitCode_EpollWait = 10
} ExitCode;

// The file descriptors are initialized to an invalid value so they can
// be cleaned up safely if they are only partially initialized.
static int epollFd = -1;
static int nrfUartFd = -1;
static int nrfResetGpioFd = -1;
static int nrfDfuModeGpioFd = -1;
static int triggerUpdateButtonGpioFd = -1;
static int buttonPollTimerFd = -1;

// State variables
static GPIO_Value_Type buttonState = GPIO_Value_High;

// To write an image to the Nordic board, add the data and binary files as
// resources to the solution and modify this object. The first image should
// be the softdevice; the second image is the application.
static DfuImageData images[] = {
    {.datPathname = "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.dat",
     .binPathname = "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.bin",
     .firmwareType = DfuFirmware_Softdevice,
     .version = 6001000},
    {.datPathname = "ExternalNRF52Firmware/blinkyV1.dat",
     .binPathname = "ExternalNRF52Firmware/blinkyV1.bin",
     .firmwareType = DfuFirmware_Application,
     .version = 1}};

static const size_t imageCount = sizeof(images) / sizeof(images[0]);

// Whether currently writing images to attached board.
static bool inDfuMode = false;

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

void DfuTerminationHandler(DfuResultStatus status)
{
    Log_Debug("\nFinished updating images with status: %s, setting DFU mode to false.\n",
              status == DfuResult_Success ? "SUCCESS" : "FAILED");
    inDfuMode = false;
}

/// <summary>
///     Handle button timer event: if the button is pressed, trigger DFU mode and send updates.
/// </summary>
static void ButtonPollTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) {
        exitCode = ExitCode_ButtonTimerHandler_Consume;
        return;
    }
    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(triggerUpdateButtonGpioFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimerHandler_GetValue;
        return;
    }

    // If the button has just been pressed and we are not in DFU mode, we trigger the update.
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released.
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {
            if (!inDfuMode) {
                Log_Debug("\nStarting firmware update...\n");
                inDfuMode = true;
                ProgramImages(images, imageCount, &DfuTerminationHandler);
            }
        }
        buttonState = newButtonState;
    }
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonPollTimerEvent = {.eventHandler = &ButtonPollTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>ExitCode_Success if all resources were allocated successfully; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    nrfResetGpioFd =
        GPIO_OpenAsOutput(SAMPLE_NRF52_RESET, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
    if (nrfResetGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_NRF52_RESET: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Reset;
    }
    GPIO_SetValue(nrfResetGpioFd, GPIO_Value_Low);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd == -1) {
        return ExitCode_Init_Epoll;
    }

    // Create a UART_Config object, open the UART and set up UART event handler
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_RTSCTS;
    nrfUartFd = UART_Open(SAMPLE_NRF52_UART, &uartConfig);
    if (nrfUartFd == -1) {
        Log_Debug("ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Uart;
    }
    // uartFd will be added to the epoll when needed (check epoll protocol)

    nrfDfuModeGpioFd =
        GPIO_OpenAsOutput(SAMPLE_NRF52_DFU, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
    if (nrfDfuModeGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_NRF52_DFU: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_DfuMode;
    }

    InitUartProtocol(nrfUartFd, nrfResetGpioFd, nrfDfuModeGpioFd, epollFd);

    Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
    triggerUpdateButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (triggerUpdateButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Trigger;
    }

    struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod,
                                                   &buttonPollTimerEvent, EPOLLIN);
    if (buttonPollTimerFd == -1) {
        return ExitCode_Init_ButtonTimer;
    }

    // Take nRF52 out of reset, allowing its application to start
    GPIO_SetValue(nrfResetGpioFd, GPIO_Value_High);

    Log_Debug("\nStarting firmware update...\n");
    inDfuMode = true;
    ProgramImages(images, imageCount, &DfuTerminationHandler);

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors\n");
    CloseFdAndPrintError(buttonPollTimerFd, "ButtonPollTimer");
    CloseFdAndPrintError(triggerUpdateButtonGpioFd, "TriggerUpdateButtonGpio");
    CloseFdAndPrintError(nrfResetGpioFd, "NrfResetGpio");
    CloseFdAndPrintError(nrfDfuModeGpioFd, "NrfDfuModeGpio");
    CloseFdAndPrintError(nrfUartFd, "NrfUart");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("DFU firmware update application\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            exitCode = ExitCode_EpollWait;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting\n");
    return exitCode;
}
