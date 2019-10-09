/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "mt3620-baremetal.h"
#include "mt3620-uart-poll.h"

static const uintptr_t UART_BASE = 0x21040000;

static void WriteIntegerAsStringWidth(int value, int width);

void Uart_Init(void)
{
    // Configure UART to use 115200-8-N-1.
    WriteReg32(UART_BASE, 0x0C, 0x80); // LCR (enable DLL, DLM)
    WriteReg32(UART_BASE, 0x24, 0x3);  // HIGHSPEED
    WriteReg32(UART_BASE, 0x04, 0);    // Divisor Latch (MS)
    WriteReg32(UART_BASE, 0x00, 1);    // Divisor Latch (LS)
    WriteReg32(UART_BASE, 0x28, 224);  // SAMPLE_COUNT
    WriteReg32(UART_BASE, 0x2C, 110);  // SAMPLE_POINT
    WriteReg32(UART_BASE, 0x58, 0);    // FRACDIV_M
    WriteReg32(UART_BASE, 0x54, 223);  // FRACDIV_L
    WriteReg32(UART_BASE, 0x0C, 0x03); // LCR (8-bit word length)
}

void Uart_WriteStringPoll(const char *msg)
{
    while (*msg) {
        // When LSR[5] is set, can write another character.
        while (!(ReadReg32(UART_BASE, 0x14) & (1U << 5))) {
            // empty.
        }

        WriteReg32(UART_BASE, 0x0, *msg++);
    }
}

static void WriteIntegerAsStringWidth(int value, int width)
{
    // Maximum decimal length is minus sign, ten digits, and null terminator.
    char txt[1 + 10 + 1];
    char *p = txt;

    bool isNegative = value < 0;
    char *numStart = txt;
    if (isNegative) {
        *p++ = '-';
        ++numStart;
    }

    static const int base = 10;
    static const char digits[] = "0123456789";
    do {
        *p++ = digits[__builtin_abs(value % base)];
        value /= base;
    } while (value && ((width == -1) || (p - numStart < width)));

    // Append '0' if required to reach width.
    if (width != -1 && p - numStart < width) {
        int requiredZeroes = width - (p - numStart);
        __builtin_memset(p, '0', requiredZeroes);
        p += requiredZeroes;
    }

    *p = '\0';

    // Reverse the digits, not including any negative sign.
    char *low = numStart;
    char *high = p - 1;
    while (low < high) {
        char tmp = *low;
        *low = *high;
        *high = tmp;
        ++low;
        --high;
    }

    return Uart_WriteStringPoll(txt);
}

void Uart_WriteIntegerPoll(int value)
{
    WriteIntegerAsStringWidth(value, -1);
}

void Uart_WriteIntegerWidthPoll(int value, int width)
{
    WriteIntegerAsStringWidth(value, width);
}

void Uart_WriteHexBytePoll(uint8_t value)
{
    static const char digits[] = "0123456789abcdef";

    char text[3];
    text[0] = digits[value >> 4];
    text[1] = digits[value & 0xF];
    text[2] = '\0';

    Uart_WriteStringPoll(text);
}
