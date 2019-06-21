/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "mt3620-baremetal.h"
#include "mt3620-uart-poll.h"

static const uintptr_t UART_BASE = 0x21040000;

static void WriteIntegerAsStringWithBaseWidth(unsigned int val, unsigned int base, int width);

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

static void WriteIntegerAsStringWithBaseWidth(unsigned int val, unsigned int base, int width)
{
	// Maximum decimal length is minus sign, ten digits, and null terminator.
#define MAXDIGITS 10
	char txt[1 + MAXDIGITS + 1];
	// Start pointer at end of buffer and write backwards.
	char *p = txt + sizeof(txt) - 1;
	int cnt = 0;

	// Limit width and base
	if (width > MAXDIGITS) width = MAXDIGITS;
	if (base > 16) base = 16;
	if (base == 0) base = 10;

	// Terminate end of string
	*p-- = '\0';

	bool isNegative = false;
	if ((val & 0x80000000) && (base == 10)) {
		isNegative = true;
		val = __builtin_abs(val);
	}
	// Convert each digit.
	static const char digits[] = "0123456789abcdef";
	do {
		*p-- = digits[(val % base)];
		val /= base;
		cnt++;
	} while ((val && (width == -1)) || (cnt < width));
	// Prepend the sign if needed.
	if (isNegative)
		*p-- = '-';
	// Move forward to first character.
	p++;
	return Uart_WriteStringPoll(p);
}

void Uart_WriteIntegerPoll(int value)
{
    WriteIntegerAsStringWithBaseWidth(value, 10, -1);
}

void Uart_WriteIntegerWidthPoll(int value, int width)
{
    WriteIntegerAsStringWithBaseWidth(value, 10, width);
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
