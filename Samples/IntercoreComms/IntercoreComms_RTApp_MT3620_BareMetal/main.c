/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for the real-time core demonstrates intercore communications by
// reading a message from the high-level core, printing it out, modifying it, and then sending
// it back to the high-level core.
//
// It demontrates the following hardware
// - UART (used to write a message via the built-in UART)
// - mailbox (used to report buffer sizes and send / receive events)

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"
#include "mt3620-uart-poll.h"

extern uint32_t StackTop; // &StackTop == end of TCM

static _Noreturn void DefaultExceptionHandler(void);

static void PrintBytes(const uint8_t *buf, int start, int end);
static void PrintGuid(const uint8_t *guid);

static _Noreturn void RTCoreMain(void);

// ARM DDI0403E.d SB1.5.2-3
// From SB1.5.3, "The Vector table must be naturally aligned to a power of two whose alignment
// value is greater than or equal to (Number of Exceptions supported x 4), with a minimum alignment
// of 128 bytes.". The array is aligned in linker.ld, using the dedicated section ".vector_table".

// The exception vector table contains a stack pointer, 15 exception handlers, and an entry for
// each interrupt.
#define INTERRUPT_COUNT 100 // from datasheet
#define EXCEPTION_COUNT (16 + INTERRUPT_COUNT)
#define INT_TO_EXC(i_) (16 + (i_))
const uintptr_t ExceptionVectorTable[EXCEPTION_COUNT] __attribute__((section(".vector_table")))
__attribute__((used)) = {
    [0] = (uintptr_t)&StackTop,                // Main Stack Pointer (MSP)
    [1] = (uintptr_t)RTCoreMain,               // Reset
    [2] = (uintptr_t)DefaultExceptionHandler,  // NMI
    [3] = (uintptr_t)DefaultExceptionHandler,  // HardFault
    [4] = (uintptr_t)DefaultExceptionHandler,  // MPU Fault
    [5] = (uintptr_t)DefaultExceptionHandler,  // Bus Fault
    [6] = (uintptr_t)DefaultExceptionHandler,  // Usage Fault
    [11] = (uintptr_t)DefaultExceptionHandler, // SVCall
    [12] = (uintptr_t)DefaultExceptionHandler, // Debug monitor
    [14] = (uintptr_t)DefaultExceptionHandler, // PendSV
    [15] = (uintptr_t)DefaultExceptionHandler, // SysTick

    [INT_TO_EXC(0)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler};

static _Noreturn void DefaultExceptionHandler(void)
{
    for (;;) {
        // empty.
    }
}

static void PrintBytes(const uint8_t *buf, int start, int end)
{
    int step = (end >= start) ? +1 : -1;

    for (/* nop */; start != end; start += step) {
        Uart_WriteHexBytePoll(buf[start]);
    }
    Uart_WriteHexBytePoll(buf[end]);
}

static void PrintGuid(const uint8_t *guid)
{
    PrintBytes(guid, 3, 0); // 4-byte little-endian word
    Uart_WriteStringPoll("-");
    PrintBytes(guid, 5, 4); // 2-byte little-endian half
    Uart_WriteStringPoll("-");
    PrintBytes(guid, 7, 6); // 2-byte little-endian half
    Uart_WriteStringPoll("-");
    PrintBytes(guid, 8, 9); // 2 bytes
    Uart_WriteStringPoll("-");
    PrintBytes(guid, 10, 15); // 6 bytes
}

static _Noreturn void RTCoreMain(void)
{
    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    Uart_Init();
    Uart_WriteStringPoll("--------------------------------\r\n");
    Uart_WriteStringPoll("IntercoreComms_RTApp_MT3620_BareMetal\r\n");
    Uart_WriteStringPoll("App built on: " __DATE__ ", " __TIME__ "\r\n");

    BufferHeader *outbound, *inbound;
    uint32_t sharedBufSize = 0;
    if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {
        for (;;) {
            // empty.
        }
    }

    static const size_t payloadStart = 20;

    for (;;) {
        uint8_t buf[256];
        uint32_t dataSize = sizeof(buf);

        // On success, dataSize is set to the actual number of bytes which were read.
        int r = DequeueData(outbound, inbound, sharedBufSize, buf, &dataSize);

        if (r == -1 || dataSize < payloadStart) {
            continue;
        }

        Uart_WriteStringPoll("Received message of ");
        Uart_WriteIntegerPoll(dataSize);
        Uart_WriteStringPoll(" bytes:\r\n");

        Uart_WriteStringPoll("  Component ID (16 bytes): ");
        PrintGuid(buf);
        Uart_WriteStringPoll("\r\n");

        // Print reserved field as little-endian 4-byte integer.
        Uart_WriteStringPoll("  Reserved (4 bytes): ");
        PrintBytes(buf, 19, 16);
        Uart_WriteStringPoll("\r\n");

        // Print message as hex.
        size_t payloadBytes = dataSize - payloadStart;
        Uart_WriteStringPoll("  Payload (");
        Uart_WriteIntegerPoll(payloadBytes);
        Uart_WriteStringPoll(" bytes as hex): ");

        for (size_t i = payloadStart; i < dataSize; ++i) {
            Uart_WriteHexBytePoll(buf[i]);
            if (i != dataSize - 1) {
                Uart_WriteStringPoll(":");
            }
        }
        Uart_WriteStringPoll("\r\n");

        // Print message as text.
        Uart_WriteStringPoll("  Payload (");
        Uart_WriteIntegerPoll(payloadBytes);
        Uart_WriteStringPoll(" bytes as text): ");
        for (size_t i = payloadStart; i < dataSize; ++i) {
            char c[2];
            c[0] = isprint(buf[i]) ? buf[i] : '.';
            c[1] = '\0';
            Uart_WriteStringPoll(c);
        }
        Uart_WriteStringPoll("\r\n");

        // Transform the payload by converting upper-case text to lower-case and vice versa,
        // and send the payload back to the sender.
        for (size_t i = payloadStart; i < dataSize; ++i) {
            // This must be an unsigned char, rather than a char, else a compile-time warning
            // is triggered by __ctype_lookup in ctype.h.
            unsigned char c = buf[i];
            if (isupper(c)) {
                c = tolower(c);
            } else if (islower(c)) {
                c = toupper(c);
            }

            buf[i] = c;
        }

        EnqueueData(inbound, outbound, sharedBufSize, buf, dataSize);
    }
}
