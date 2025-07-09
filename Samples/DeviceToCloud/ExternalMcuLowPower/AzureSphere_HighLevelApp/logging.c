/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/log.h>

#include <stdarg.h>

#include "debug_uart.h"

/// <summary>
///     Overrides the AppLibs <see cref="Log_Debug" /> to also write to the debug UART
/// </summary>
/// <param name="fmt">String to write to the log and optional parameters</param>
/// <returns>
///     0 for success, or -1 on failure (with errno set to indicate the error, as per
///     <see cref="Log_Debug" />)
/// </returns>
int Log_Debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int result = Log_DebugVarArgs(fmt, args);
    DebugUart_LogVarArgs(fmt, args);

    va_end(args);

    return result;
}
