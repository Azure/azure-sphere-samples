/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for the real-time core demonstrates intercore communications by
// sending a message to a high-level application every second, and printing out any received
// messages.
//
// It demontrates the following hardware
// - UART (used to write a message via the built-in UART)
// - mailbox (used to report buffer sizes and send / receive events)
// - timer (used to send a message to the HLApp)

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "logical-dpc.h"
#include "logical-intercore.h"

#include "mt3620-baremetal.h"
#include "mt3620-uart-poll.h"
#include "mt3620-intercore.h"
#include "mt3620-timer.h"

extern uint32_t StackTop; // &StackTop == end of TCM

static IntercoreComm icc;

static const uint32_t sendTimerIntervalMs = 1000;

static _Noreturn void DefaultExceptionHandler(void);
static void HandleSendTimerIrq(void);
static void HandleSendTimerDeferred(void);

static void PrintBytes(const void *buf, int start, int end);
static void PrintGuid(const ComponentId *cid);

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

    [INT_TO_EXC(0)] = (uintptr_t)DefaultExceptionHandler,
    [INT_TO_EXC(1)] = (uintptr_t)MT3620_Gpt_HandleIrq1,
    [INT_TO_EXC(2)... INT_TO_EXC(10)] = (uintptr_t)DefaultExceptionHandler,
    [INT_TO_EXC(11)] = (uintptr_t)MT3620_HandleMailboxIrq11,
    [INT_TO_EXC(12)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler};

// If the applications end up in this function then an unexpected exception has occurred.
static _Noreturn void DefaultExceptionHandler(void)
{
    for (;;) {
        // empty.
    }
}

// Runs in IRQ context and schedules HandleSendTimerDeferred to run later.
static void HandleSendTimerIrq(void)
{
    static CallbackNode cbn = {.enqueued = false, .cb = HandleSendTimerDeferred};
    EnqueueDeferredProc(&cbn);
}

// Queued by HandleSendTimerIrq. Sends a message to the HLApp.
static void HandleSendTimerDeferred(void)
{
    static int iter = 0;
    // The component ID for IntercoreComms_HighLevelApp.
    static const ComponentId hlAppId = {.data1 = 0x25025d2c,
                                        .data2 = 0x66da,
                                        .data3 = 0x4448,
                                        .data4 = {0xba, 0xe1, 0xac, 0x26, 0xfc, 0xdd, 0x36, 0x27}};

    // The number cycles from "00" to "99".
    static char txMsg[] = "rt-app-to-hl-app-00";
    const size_t txMsgLen = sizeof(txMsg);

    IntercoreResult icr = IntercoreSend(&icc, &hlAppId, txMsg, sizeof(txMsg) - 1);
    if (icr != Intercore_OK) {
        Uart_WriteStringPoll("IntercoreSend: ");
        Uart_WriteIntegerPoll(icr);
        Uart_WriteStringPoll("\r\n");
    }

    txMsg[txMsgLen - 3] = '0' + (iter / 10);
    txMsg[txMsgLen - 2] = '0' + (iter % 10);
    iter = (iter + 1) % 100;

    MT3620_Gpt_LaunchTimerMs(TimerGpt0, sendTimerIntervalMs, HandleSendTimerIrq);
}

// Prints a sequence of bytes. If the start position occurs after the end
// position, they are printed in reverse order. Therefore, this function can
// print big- or little-endian values.
static void PrintBytes(const void *buf, int start, int end)
{
    const uint8_t *buf8 = (const uint8_t *)buf;
    int step = (end >= start) ? +1 : -1;

    for (/* nop */; start != end; start += step) {
        Uart_WriteHexBytePoll(buf8[start]);
    }
    Uart_WriteHexBytePoll(buf8[end]);
}

// Renders the supplied component ID as a string "00112233-4455-6677-8899-aabbccddeeff".
// The first three values are interpreted as little-endian, and last two as big-endian.
static void PrintGuid(const ComponentId *cid)
{
    PrintBytes(&cid->data1, 3, 0); // 4-byte little-endian word
    Uart_WriteStringPoll("-");
    PrintBytes(&cid->data2, 1, 0); // 2-byte little-endian half
    Uart_WriteStringPoll("-");
    PrintBytes(&cid->data3, 1, 0); // 2-byte little-endian half
    Uart_WriteStringPoll("-");
    PrintBytes(&cid->data4, 0, 1); // 2 bytes
    Uart_WriteStringPoll("-");
    PrintBytes(&cid->data4, 2, 7); // 6 bytes
}

// Runs with interrupts enabled. Retrieves messages from the inbound buffer
// and prints their sender ID, length, and content (hex and text).
static void HandleReceivedMessageDeferred(void)
{
    for (;;) {
        ComponentId sender;
        uint8_t rxData[32];
        size_t rxDataSize = sizeof(rxData);

        IntercoreResult icr = IntercoreRecv(&icc, &sender, rxData, &rxDataSize);

        // Return if read all messages in buffer.
        if (icr == Intercore_Recv_NoBlockSize) {
            return;
        }

        // Return if an error occurred.
        if (icr != Intercore_OK) {
            Uart_WriteStringPoll("IntercoreRecv: ");
            Uart_WriteIntegerPoll(icr);
            Uart_WriteStringPoll("\r\n");
            return;
        }

        // Display sender component ID.
        Uart_WriteStringPoll("Sender: ");
        PrintGuid(&sender);
        Uart_WriteStringPoll("\r\n");

        Uart_WriteStringPoll("Message size: ");
        Uart_WriteIntegerPoll((int)rxDataSize);
        Uart_WriteStringPoll(" bytes:\r\n");

        // Print message as hex.
        Uart_WriteStringPoll("Hex: ");
        for (uint32_t i = 0; i < rxDataSize; ++i) {
            Uart_WriteHexBytePoll(rxData[i]);
            if (i != rxDataSize - 1) {
                Uart_WriteStringPoll(":");
            }
        }
        Uart_WriteStringPoll("\r\n");

        // Print message as text.
        Uart_WriteStringPoll("Text: ");
        for (uint32_t i = 0; i < rxDataSize; ++i) {
            char c[2];
            c[0] = isprint(rxData[i]) ? rxData[i] : '.';
            c[1] = '\0';
            Uart_WriteStringPoll(c);
        }
        Uart_WriteStringPoll("\r\n");
    }
}

static _Noreturn void RTCoreMain(void)
{
    // The debugger will not connect until shortly after the application has started running.
    // To use the debugger with code which runs at application startup, change the initial value
    // of b from true to false, run the app, break into the app with a debugger, and set b to true.
    volatile bool b = true;
    while (!b) {
        // empty.
    }

    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    Uart_Init();
    Uart_WriteStringPoll("--------------------------------\r\n");
    Uart_WriteStringPoll("IntercoreComms_RTApp_MT3620_BareMetal\r\n");
    Uart_WriteStringPoll("App built on: " __DATE__ ", " __TIME__ "\r\n");

    MT3620_Gpt_Init();

    IntercoreResult icr = SetupIntercoreComm(&icc, HandleReceivedMessageDeferred);
    if (icr != Intercore_OK) {
        Uart_WriteStringPoll("SetupIntercoreComm: ");
        Uart_WriteIntegerPoll(icr);
        Uart_WriteStringPoll("\r\n");
    } else {
        MT3620_Gpt_LaunchTimerMs(TimerGpt0, sendTimerIntervalMs, HandleSendTimerIrq);
    }

    for (;;) {
        InvokeDeferredProcs();
        __asm__("wfi");
    }
}
