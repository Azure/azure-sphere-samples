/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define UART_STRUCTS_VERSION 1
#include <applibs/eventloop.h>
#include <applibs/uart.h>
#include <applibs/log.h>

#include "uart_transport.h"
#include "exitcodes.h"

#define UART_SEND_BUFFER_SIZE 247u // This is the max MTU size of BLE GATT.

static EventLoop *eventLoopRef = NULL;
static int messageUartFd = -1;

static UartTransport_DataReadyCallback dataReadyCallback = NULL;

// True if the UART event is registered for EventLoop_Output; false if EventLoop_Input
static bool uartEventOutputEnabled = false;

// Buffer for data to be writtern via UART.
static uint8_t sendBuffer[UART_SEND_BUFFER_SIZE];

// Total amount of data in sendBuffer.
static size_t sendBufferDataLength = 0;

// Amount of data so far written to the UART.
static size_t sendBufferDataSent = 0;

static EventRegistration *uartEventRegistration = NULL;

static void UartEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void SendUartMessage(void);

/// <summary>
/// Handle events from the UART fd.
/// </summary>
static void UartEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    switch (events) {
    case EventLoop_Input:
        dataReadyCallback();
        break;

    case EventLoop_Output:
        SendUartMessage();
        break;

    default:
        Log_Debug("ERROR: Unexpected UART IO event %u\n", events);
        break;
    }
}

/// <summary>
/// Attempt to send the data in <see cref="sendBuffer" /> over the UART. If this fails because the
/// device is busy (as indicated by EAGAIN), then we modify the UART event registration to wait for
/// EventLoop_Output, indicating the device is ready again and we should retry writing.
/// </summary>
static void SendUartMessage(void)
{
    if (uartEventOutputEnabled) {
        EventLoop_ModifyIoEvents(eventLoopRef, uartEventRegistration, EventLoop_Input);
        uartEventOutputEnabled = false;
    }

    while (sendBufferDataSent < sendBufferDataLength) {
        // Send as much of the remaining data as possible.
        size_t bytesLeftToSend = (size_t)(sendBufferDataLength - sendBufferDataSent);
        const uint8_t *remainingMessageToSend = sendBuffer + sendBufferDataSent;
        ssize_t bytesSent = write(messageUartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent == -1) {
            if (errno != EAGAIN) {
                Log_Debug("ERROR: Failed to write to UART: %s (%d).\n", strerror(errno), errno);
            } else {
                // Register an output event to send the rest
                EventLoop_ModifyIoEvents(eventLoopRef, uartEventRegistration, EventLoop_Output);
                uartEventOutputEnabled = true;
            }
            return;
        }
        sendBufferDataSent += (size_t)bytesSent;
    }
}

ssize_t UartTransport_Read(char *buffer, size_t amount)
{
    return read(messageUartFd, (void *)buffer, amount);
}

ssize_t UartTransport_Send(const char *buffer, size_t length)
{
    if (length > UART_SEND_BUFFER_SIZE) {
        return 0;
    }

    if (messageUartFd == -1) {
        return 0;
    }

    memcpy(sendBuffer, buffer, length);

    sendBufferDataLength = length;
    sendBufferDataSent = 0;
    SendUartMessage();

    return (ssize_t)length;
}

ExitCode UartTransport_Initialize(EventLoop *eventLoop, UART_Id uartId,
                                  UartTransport_DataReadyCallback uartDataReadyCallback)
{
    eventLoopRef = eventLoop;

    UART_Config config;
    UART_InitConfig(&config);
    config.baudRate = 115200;
    config.dataBits = 8;
    config.parity = UART_Parity_None;
    config.stopBits = 1;
    config.flowControl = UART_FlowControl_None;

    messageUartFd = UART_Open(uartId, &config);
    if (messageUartFd == -1) {
        Log_Debug("ERROR: Failed to open UART: %s (%d)\n", strerror(errno), errno);
        return ExitCode_Uart_Init_OpenFail;
    }

    dataReadyCallback = uartDataReadyCallback;

    uartEventRegistration =
        EventLoop_RegisterIo(eventLoop, messageUartFd, EventLoop_Input, UartEventHandler, NULL);
    if (uartEventRegistration == NULL) {
        Log_Debug("ERROR: Failed to register UART fd to event loop: %s (%d)", strerror(errno),
                  errno);
        return ExitCode_Uart_Init_EventRegisterFail;
    }

    return ExitCode_Success;
}

void UartTransport_Cleanup(void)
{
    if (uartEventRegistration != NULL) {
        int result = EventLoop_UnregisterIo(eventLoopRef, uartEventRegistration);
        if (result == -1) {
            Log_Debug("ERROR: Failed to unregister UART from event loop: %s (%d)", strerror(errno),
                      errno);
        }
    }

    if (messageUartFd != -1) {
        int result = close(messageUartFd);
        if (result == -1) {
            Log_Debug("ERROR: Failed to close UART fd: %s (%d)", strerror(errno), errno);
        }
    }

    eventLoopRef = NULL;
    dataReadyCallback = NULL;
}
