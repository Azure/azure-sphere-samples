/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern uint32_t StackTop; // &StackTop == end of TCM0

static const uintptr_t GPT_BASE = 0x21030000;
static const uintptr_t UART_BASE = 0x21040000;
static const uintptr_t SCB_BASE = 0xE000ED00;

static _Noreturn void DefaultExceptionHandler(void);

static void WriteReg32(uintptr_t baseAddr, size_t offset, uint32_t value);
static uint32_t ReadReg32(uintptr_t baseAddr, size_t offset);

static void Uart_Init(void);
static void Uart_WritePoll(const char *msg);

static void Gpt3_WaitUs(int microseconds);

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

static void WriteReg32(uintptr_t baseAddr, size_t offset, uint32_t value)
{
    *(volatile uint32_t *)(baseAddr + offset) = value;
}

static uint32_t ReadReg32(uintptr_t baseAddr, size_t offset)
{
    return *(volatile uint32_t *)(baseAddr + offset);
}

static void Uart_Init(void)
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

static void Uart_WritePoll(const char *msg)
{
    while (*msg) {
        // When LSR[5] is set, can write another character.
        while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 5))) {
            // empty.
        }

        WriteReg32(UART_BASE, 0x0, *msg++);
    }
}

static void Gpt3_WaitUs(int microseconds)
{
    // GPT3_INIT = initial counter value
    WriteReg32(GPT_BASE, 0x54, 0x0);

    // GPT3_CTRL
    uint32_t ctrlOn = 0x0;
    ctrlOn |= (0x19) << 16; // OSC_CNT_1US (default value)
    ctrlOn |= 0x1;          // GPT3_EN = 1 -> GPT3 enabled
    WriteReg32(GPT_BASE, 0x50, ctrlOn);

    // GPT3_CNT
    while (ReadReg32(GPT_BASE, 0x58) < microseconds) {
        // empty.
    }

    // GPT_CTRL -> disable timer
    WriteReg32(GPT_BASE, 0x50, 0x0);
}

static _Noreturn void RTCoreMain(void)
{
    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    Uart_Init();

    // This minimal Azure Sphere app repeatedly prints "Tick" then "Tock" to the
    // debug UART, at one second intervals. Use this app to test the device and SDK
    // installation succeeded, and that you can deploy and debug applications on the
    // real-time core.

    static const int tickPeriodUs = 1 * 1000 * 1000;
    while (true) {
        Uart_WritePoll("Tick\r\n");
        Gpt3_WaitUs(tickPeriodUs);
        Uart_WritePoll("Tock\r\n");
        Gpt3_WaitUs(tickPeriodUs);
    }
}
