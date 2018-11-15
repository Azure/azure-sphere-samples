/* Copyright (c) Microsoft Corporation. All rights reserved.
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

#include "mt3620_rdb.h"

#include "nordic/dfu_uart_protocol.h"

// The file descriptors are initialized to an invalid value so they can
// be cleaned up safely if they are only partially initialized.
static int epollFd = -1;
static int uartFd = -1;
static int resetFd = -1;
static int gpioNrfDfuModeFd = -1;
static int gpioButtonFd = -1;
static int dfuButtonTimerFd = -1;

// State variables
static GPIO_Value_Type buttonState = GPIO_Value_High;

// To write an image to the Nordic board, add the data and binary files as 
// resources to the solution and modify this object. The first image should
// be the softdevice; the second image is the application.
static DfuImageData images[] = {
    {
        .datPathname = "s132_nrf52_6.1.0_softdevice.dat",
        .binPathname = "s132_nrf52_6.1.0_softdevice.bin",
        .firmwareType = DfuFirmware_Softdevice,
        .version = 6001000,
        .needsUpdate = true
    },
    {
        .datPathname = "blinkyV1.dat",
        .binPathname = "blinkyV1.bin",
        .firmwareType = DfuFirmware_Application,
        .version = 1,
        .needsUpdate = true
    }
};

static const size_t imageCount = sizeof(images) / sizeof(images[0]);

// Whether currently writing images to attached board.
static bool inDfuMode = false;

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

void DfuTerminationHandler(DfuResultStatus status)
{
    Log_Debug("\nFinished updating images with status: %s, setting DFU mode to false.\n",
              status == DfuResult_Success ? "SUCCESS" : "FAILED");
    inDfuMode = false;
}

/// <summary>
///     Handle button timer event: if the button is pressed, trigger DFU mode and send updates.
/// </summary>
static void DfuTimerEventHandler(struct event_data *eventData)
{
    if (ConsumeTimerFdEvent(dfuButtonTimerFd) != 0) {
        terminationRequired = true;
        return;
    }
    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(gpioButtonFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        terminationRequired = true;
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
static event_data_t dfuButtonTimerEvent = {.eventHandler = &DfuTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    resetFd =
        GPIO_OpenAsOutput(MT3620_GPIO5, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
    if (resetFd == -1) {
        Log_Debug("ERROR: Could not open MT3620_GPIO5: %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    GPIO_SetValue(resetFd, GPIO_Value_Low);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd == -1) {
        return -1;
    }

    // Create a UART_Config object, open the UART and set up UART event handler
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_RTSCTS;
    uartFd = UART_Open(MT3620_RDB_HEADER2_ISU0_UART, &uartConfig);
    if (uartFd == -1) {
        Log_Debug("ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    // uartFd will be added to the epoll when needed (check epoll protocol)

    gpioNrfDfuModeFd = GPIO_OpenAsOutput(MT3620_GPIO44, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
    if (gpioNrfDfuModeFd == -1) {
        Log_Debug("ERROR: Could not open MT3620_GPIO44: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    InitUartProtocol(uartFd, resetFd, gpioNrfDfuModeFd, epollFd);
    
	Log_Debug("Opening MT3620_RDB_BUTTON_A as input\n");
    gpioButtonFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
    if (gpioButtonFd == -1) {
        Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    struct timespec buttonPressCheckPeriod = {0, 1000000};
    dfuButtonTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &dfuButtonTimerEvent, EPOLLIN);
    if (dfuButtonTimerFd == -1) {
        return -1;
    }

    // Take nRF52 out of reset, allowing its application to start
    GPIO_SetValue(resetFd, GPIO_Value_High);

    Log_Debug("\nStarting firmware update...\n");
	inDfuMode = true;
    ProgramImages(images, imageCount, &DfuTerminationHandler);

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors\n");
    CloseFdAndPrintError(dfuButtonTimerFd, "dfuButtonTimerFd");
    CloseFdAndPrintError(gpioButtonFd, "gpioButtonFd");
    CloseFdAndPrintError(resetFd, "resetFd");
    CloseFdAndPrintError(gpioNrfDfuModeFd, "gpioNrfDfuModeFd");
    CloseFdAndPrintError(uartFd, "Uart");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("DFU firmware update application\n");
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
    Log_Debug("Application exiting\n");
    return 0;
}
