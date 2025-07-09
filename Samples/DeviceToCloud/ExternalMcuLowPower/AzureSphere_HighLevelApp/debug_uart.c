/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

#include "applibs_versions.h"
#include <applibs/uart.h>

#include <hw/soda_machine.h>

static int uartFd = -1;

void DebugUart_Init(void)
{
    UART_Config uartConfig;

    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.dataBits = UART_DataBits_Eight;
    uartConfig.stopBits = UART_StopBits_One;
    uartConfig.parity = UART_Parity_None;
    uartConfig.flowControl = UART_FlowControl_None;

    uartFd = UART_Open(SODAMACHINE_DEBUG_UART, &uartConfig);
}

void DebugUart_Cleanup(void)
{
    if (uartFd != -1) {
        close(uartFd);
    }
}

void DebugUart_LogVarArgs(const char *fmt, va_list args)
{
    if (uartFd == -1) {
        return;
    }

    char *string;
    int length = vasprintf(&string, fmt, args);

    if (length != -1) {
        write(uartFd, string, (size_t)length);
        free(string);
    }
}

void DebugUart_Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugUart_LogVarArgs(fmt, args);
    va_end(args);
}
