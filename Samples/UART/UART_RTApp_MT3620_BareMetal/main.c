/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "mt3620-baremetal.h"
#include "mt3620-timer.h"
#include "mt3620-gpio.h"
#include "mt3620-uart.h"

extern uint32_t StackTop; // &StackTop == end of TCM0

_Noreturn static void DefaultExceptionHandler(void);

static const int buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(void);
static void HandleButtonTimerIrqDeferred(void);

static void HandleUartIsu0RxIrq(void);
static void HandleUartIsu0RxIrqDeferred(void);

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    Callback cb;
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

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
static const uintptr_t ExceptionVectorTable[EXCEPTION_COUNT]
    __attribute__((section(".vector_table"))) __attribute__((used)) = {
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
        [INT_TO_EXC(1)] = (uintptr_t)Gpt_HandleIrq1,
        [INT_TO_EXC(2)... INT_TO_EXC(3)] = (uintptr_t)DefaultExceptionHandler,
        [INT_TO_EXC(4)] = (uintptr_t)Uart_HandleIrq4,
        [INT_TO_EXC(5)... INT_TO_EXC(46)] = (uintptr_t)DefaultExceptionHandler,
        [INT_TO_EXC(47)] = (uintptr_t)Uart_HandleIrq47,
        [INT_TO_EXC(48)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler};

static _Noreturn void DefaultExceptionHandler(void)
{
    for (;;) {
        // empty.
    }
}

static void HandleButtonTimerIrq(void)
{
    static CallbackNode cbn = {.enqueued = false, .cb = HandleButtonTimerIrqDeferred};
    EnqueueCallback(&cbn);
}

static void HandleButtonTimerIrqDeferred(void)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    Mt3620_Gpio_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            Uart_EnqueueString(UartIsu0, "RTCore: Hello world!\r\n");
        }

        prevState = newState;
    }

    Gpt_LaunchTimerMs(TimerGpt1, buttonPressCheckPeriodMs, HandleButtonTimerIrq);
}

static void HandleUartIsu0RxIrq(void)
{
    static CallbackNode cbn = {.enqueued = false, .cb = HandleUartIsu0RxIrqDeferred};
    EnqueueCallback(&cbn);
}

static void HandleUartIsu0RxIrqDeferred(void)
{
    uint8_t buffer[32];

    for (;;) {
        size_t availBytes = Uart_DequeueData(UartIsu0, buffer, sizeof(buffer));

        if (availBytes == 0) {
            return;
        }

        Uart_EnqueueString(UartCM4Debug, "UART received ");
        Uart_EnqueueIntegerAsString(UartCM4Debug, availBytes);
        Uart_EnqueueString(UartCM4Debug, " bytes: \'");
        Uart_EnqueueData(UartCM4Debug, buffer, availBytes);
        Uart_EnqueueString(UartCM4Debug, "\'.\r\n");
    }
}

static CallbackNode *volatile callbacks = NULL;

static void EnqueueCallback(CallbackNode *node)
{
    uint32_t prevBasePri = BlockIrqs();
    if (!node->enqueued) {
        CallbackNode *prevHead = callbacks;
        node->enqueued = true;
        callbacks = node;
        node->next = prevHead;
    }
    RestoreIrqs(prevBasePri);
}

static void InvokeCallbacks(void)
{
    CallbackNode *node;
    do {
        uint32_t prevBasePri = BlockIrqs();
        node = callbacks;
        if (node) {
            node->enqueued = false;
            callbacks = node->next;
        }
        RestoreIrqs(prevBasePri);

        if (node) {
            (*node->cb)();
        }
    } while (node);
}

static _Noreturn void RTCoreMain(void)
{
    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    Uart_Init(UartCM4Debug, NULL);
    Uart_EnqueueString(UartCM4Debug, "--------------------------------\r\n");
    Uart_EnqueueString(UartCM4Debug, "UART_RTApp_MT3620_BareMetal\r\n");
    Uart_EnqueueString(UartCM4Debug, "App built on: " __DATE__ " " __TIME__ "\r\n");
    Uart_EnqueueString(
        UartCM4Debug,
        "Install a loopback header on ISU0, and press button A to send a message.\r\n");

    Uart_Init(UartIsu0, HandleUartIsu0RxIrq);

    // Block includes buttonAGpio, GPIO12
    static const GpioBlock grp3 = {
        .baseAddr = 0x38040000, .type = GpioBlock_GRP, .firstPin = 12, .pinCount = 4};

    Mt3620_Gpio_AddBlock(&grp3);
    Mt3620_Gpio_ConfigurePinForInput(buttonAGpio);

    Gpt_Init();
    Gpt_LaunchTimerMs(TimerGpt1, buttonPressCheckPeriodMs, HandleButtonTimerIrq);

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
