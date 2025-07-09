/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdarg.h>

/// <summary>
/// Initialize the debug UART
/// </summary>
void DebugUart_Init(void);

/// <summary>
/// Cleanup the debug UART
/// </summary>
void DebugUart_Cleanup(void);

/// <summary>
/// Write a printf-formatted string to the debug UART
/// </summary>
/// <param name="fmt">The string to log, with optional argument parameters.</param>
void DebugUart_Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/// <summary>
/// Write a printf-formatted string to the debug UART (varargs form)
/// </summary>
/// <param name="fmt">The string to log.</param>
/// <param name="args">A varargs parameter list.</param>
void DebugUart_LogVarArgs(const char *fmt, va_list args);
