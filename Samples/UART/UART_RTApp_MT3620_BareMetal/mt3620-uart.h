/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_UART_H
#define MT3620_UART_H

#include <stdint.h>
#include <stddef.h>

#include "mt3620-baremetal.h"

/// <summary>Identifier for physical UARTs which the M4 can address.</summary>
typedef enum {
    /// <summary>
    /// <para>The M4 debug UART. Each M4 has its own debug UART, and the application does not need
    /// any capabilities to access this UART.</para>
    /// <para>If this UART is used, then register the <see cref="Uart_HandleIrq4" /> interrupt
    /// handler in the vector table.</para>
    /// </summary>
    UartCM4Debug = 0,
    /// <summary>
    /// <para>The UART which is available on ISU0. The application must request this UART in the
    /// application manifest.</para>
    /// <para>If this UART is used, then register the <see cref="Uart_HandleIrq47" /> interrupt
    /// handler in the vector table.</para>
    /// </summary>
    UartIsu0 = 1
} UartId;

/// <summary>The UART interrupts (and hence callbacks) run at this priority level.</summary>
static const uint32_t UART_PRIORITY = 2;

/// <summary>
/// <para>The application must call this function once before using a given UART.</para>
/// <para>The application should register the corresponding interrupt handler in the vector table.
/// See <see cref="UartId" /> for which interrupt handler should be registered for each UART.</para>
/// </summary>
/// <param name="id">Which UART to initialize.</param>
/// <param name="rxCallback">An optional callback to invoke when the UART receives data.
/// This can be NULL if the application does not want to read any data from the UART. The
/// application should call <see cref="Uart_DequeueData" /> to retrieve the data.</param>
void Uart_Init(UartId id, Callback rxCallback);

/// <summary>
/// <para>Buffers the supplied data and asynchronously writes it to the supplied UART.
/// If there is not enough space to buffer the data, then any unbuffered data will be discarded.
/// The size of the buffer is defined by the TX_BUFFER_SIZE macro in mt3620-uart.c.</para>
/// <para>To send a null-terminated string, call <see cref="Uart_EnqueueString" />.
/// To send an integer call <see cref="Uart_EnqueueIntegerAsString" /> or
/// <see cref="Uart_EnqueueIntegerAsHexString"/>.</para>
/// </summary>
/// <param name="id">Which UART to write the data to.</param>
/// <param name="data">Start of the data buffer.</param>
/// <param name="length">Length of the data in bytes.</param>
/// <returns>Whether all data was written to the internal buffer.</returns>
bool Uart_EnqueueData(UartId id, const uint8_t *data, size_t length);

/// <summary>
/// This function fills the supplied buffer with data which has been received on the UART,
/// and returns the number of bytes of data which were available.
/// </summary>
/// <param name="id">Which UART to read the data from.</param>
/// <param name="buffer">Start of buffer into which data should be written.</param>
/// <param name="bufferSize">Size of supplied buffer in bytes. If this is greater than the
/// available data, the return value will describe how much data was actually retrieved. If it
/// is less than the available data, then the buffer will be filled entirely, and the caller should
/// call this function again. This function does not block waiting to receive more data from the
/// UART.</param>
/// <returns>How many bytes were read from the UART. This can be zero.</returns>
size_t Uart_DequeueData(UartId id, uint8_t *buffer, size_t bufferSize);

/// <summary>
/// <para>Buffers the supplied string and asynchronously writes it to the supplied UART. Does not
/// send the null terminator. If there is not enough space to buffer the entire string, then the
/// remaining unbuffered section will be discarded.</para>
/// <para>See <see cref="Uart_EnqueueData" />
/// for more information about the transmit buffer.</para>
/// </summary>
/// <param name="id">Which UART to write the string to.</param>
/// <param name="msg">Null-terminated string to write to the UART.</param>
/// <returns>Whether all text was written to the internal buffer.</returns>
bool Uart_EnqueueString(UartId id, const char *msg);

/// <summary>
/// <para>Encodes the supplied integer as a string and asynchronously writes it to the supplied
/// UART. If there is not enough space to buffer the entire string, then the remaining unbuffered
/// section will be discarded.</para>
/// <para>See <see cref="Uart_EnqueueData" /> for more information about the transmit buffer.</para>
/// </summary>
/// <param name="id">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <returns>Whether all text was written to the internal buffer.</returns>
bool Uart_EnqueueIntegerAsString(UartId id, int value);

/// <summary>
/// <para>Encodes the supplied integer as a hexadecimal string and asynchronously writes it to the
/// supplied UART. If there is not enough space to buffer the entire string, then the remaining
/// unbuffered section will be discarded.</para>
/// <para>See <see cref="Uart_EnqueueData" /> for more information about the transmit buffer.</para>
/// </summary>
/// <param name="id">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <returns>Whether all text was written to the internal buffer.</returns>
bool Uart_EnqueueIntegerAsHexString(UartId id, uint32_t value);

/// <summary>
/// <para>Encodes the supplied integer as a fixed-width hexadecimal string and asynchronously
/// writes it to the supplied UART. If there is not enough space to buffer the entire string,
/// then the remaining unbuffered section will be discarded.</para>
/// <para>See <see cref="Uart_EnqueueIntegerAsHexString" />.</para>
/// </summary>
/// <param name="id">Which UART to print the value to.</param>
/// <param name="value">The value to print. This is rendered as an unsigned integer.</param>
/// <param name="width">Number of characters (nybbles) to print. If required, this function will
/// print leading zeroes. If the value cannot be represented in the supplied width then only
/// the lowest nybbles will be printed.</param>
/// <returns>Whether all text was written to the internal buffer.</returns>
bool Uart_EnqueueIntegerAsHexStringWidth(UartId id, uint32_t value, size_t width);

/// <summary>
/// Interrupt handler for <see cref="UartCM4Debug" />. The application should not call
/// this function directly, but should include it in the vector table.
/// </summary>
void Uart_HandleIrq4(void);

/// <summary>
/// Interrupt handler for <see cref="UartIsu0" />. The application should not call
/// this function directly, but should include it in the vector table.
/// </summary>
void Uart_HandleIrq47(void);

#endif // #ifndef MT3620_UART_H
