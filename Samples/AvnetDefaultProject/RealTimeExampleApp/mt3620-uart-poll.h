/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_UART_POLL_H
#define MT3620_UART_POLL_H

#include <stdint.h>

/// <summary>
/// Initialize the IOM4 debug UART. This function must be called once before
/// <see cref="Uart_WriteStringPoll" /> or <see cref="Uart_WriteHexBytePoll" />
/// are called.
/// </summary>
void Uart_Init(void);

/// <summary>
/// <para>Write a zero-terminated string to the debug UART. The zero terminator
/// is not written. This function will poll until the entire string has been written
/// to the UART.</para>
/// <para>Call <see cref="Uart_Init" /> before calling this function.</para>
/// </summary>
/// <param name="msg">Null-terminated string to write to the debug UART.</param>
void Uart_WriteStringPoll(const char *msg);

/// <summary>
/// <para>Write the decimal text representation of an integer to the debug UART.
/// This function will poll until the entire string has been written to the UART.</para>
/// <para>Call <see cref="Uart_Init" /> before calling this function.</para>
/// </summary>
/// <param name="value">Value to write to the UART.</param>
void Uart_WriteIntegerPoll(int value);

/// <summary>
/// <para>Write the fixed-width decmial representation of an integer to the debug UART.
/// This function will poll until the entire string has been written to the UART.</para>
/// <para>If the value is too large to fit into the supplied width then only the lowest
/// digits will be sent.  If the width is greater than required for the value, then the
/// text will be prepended with '0' characters.</para>
/// </summary>
/// <param name="value">Value to write to the UART.</param>
/// <param name="width">Number of decimal digits to write.</param>
void Uart_WriteIntegerWidthPoll(int value, int width);

/// <summary>
/// <para>Write a two-character hexadecimal string (i.e., in "%02x"-format) to the debug UART
/// which represents the supplied value. If the value is less than 0x10, then a leading '0'
/// character is written to the UART. This function polls until both digits have been
/// written to the UART.</para>
/// <para>Call <see cref="Uart_Init" /> before calling this function.</para>
/// </summary>
/// <param name="value">The value whose string representation is written to the UART.</param>
void Uart_WriteHexBytePoll(uint8_t value);

#endif // #ifndef MT3620_UART_POLL_H
