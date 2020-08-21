/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#define UART_STRUCTS_VERSION 1
#include <applibs/uart.h>
#include "exitcode.h"

typedef void (*UartTransport_DataReadyCallback)(void);

/// <summary>
///     Initialize the UART transport
/// </summary>
/// <param name="eventLoop">Pointer to the main application EventLoop.</param>
/// <param name="uartId">ID of the UART to open.</param>
/// <param name="receivedDataCallback">
///     Function to call when data is ready to be read from the UART.
/// </param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode UartTransport_Initialize(EventLoop *eventLoop, UART_Id uartId,
                                  UartTransport_DataReadyCallback receivedDataCallback);

/// <summary>
///     Close the UART transport - closes the device and de-registers any events from the EventLoop.
/// </summary>
void UartTransport_Cleanup(void);

/// <summary>
///     Queue data to be sent over the UART. This function will return immediately, but the
///     transfer may occur asynchronously, completing after the function returns.
/// </summary>
/// <param name="buffer">The data to be sent over the UART.</param>
/// <param name="length">Length of the data to be sent - must not be zero.</param>
/// <returns>
///     0 if the UART is not initialized or if the data is too long to send; otherwise, the length
///     of the queued data.
/// </returns>
ssize_t UartTransport_Send(const char *buffer, size_t length);

/// <summary>
///     Attempt to read data from the UART. This should be called upon receipt of a
///     <see cref="UartTransport_DataReadyCallBack" />.
/// </summary>
/// <param name="buffer">Buffer into which data read from the UART should be written.</param>
/// <param name="amount">Amount of data to attempt to read.</param>
/// <returns>The amount of data read, or -1 if the operation failed.</returns>
ssize_t UartTransport_Read(char *buffer, size_t amount);