/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "mt3620-baremetal.h"
#include "mt3620-timer.h"

static const uintptr_t GPT_BASE = 0x21030000;

static volatile Callback timerCallbacks[TIMER_GPT_COUNT] = {[TimerGpt0] = NULL, [TimerGpt1] = NULL};

typedef struct {
    size_t ctrlRegOffset;
    size_t icntRegOffset;
} GptInfo;

static const GptInfo gptRegOffsets[TIMER_GPT_COUNT] = {
    [TimerGpt0] = {.ctrlRegOffset = 0x10, .icntRegOffset = 0x14},
    [TimerGpt1] = {.ctrlRegOffset = 0x20, .icntRegOffset = 0x24}};

void Gpt_Init(void)
{
    // Enable INT1 in the NVIC. This allows the processor to receive an interrupt
    // from GPT0 or GPT1. The interrupt for the specific timer is enabled in Gpt_CallbackMs.

    // IO CM4 GPT0 timer and GPT1 timer interrupt both use INT1.
    SetNvicPriority(1, GPT_PRIORITY);
    EnableNvicInterrupt(1);
}

void Gpt_HandleIrq1(void)
{
    // GPT_ISR -> read, clear interrupts.
    uint32_t activeIrqs = ReadReg32(GPT_BASE, 0x00);
    WriteReg32(GPT_BASE, 0x00, activeIrqs);

    // Do not need to disable interrupts or timer because only used in one-shot mode.
    for (int gpt = 0; gpt < TIMER_GPT_COUNT; ++gpt) {
        uint32_t mask = UINT32_C(1) << gpt;
        if ((activeIrqs & mask) == 0) {
            continue;
        }

        timerCallbacks[gpt]();
    }
}

void Gpt_LaunchTimerMs(TimerGpt gpt, uint32_t periodMs, Callback callback)
{
    timerCallbacks[gpt] = callback;

    uint32_t mask = UINT32_C(1) << gpt;

    // GPTx_CTRL[0] = 0 -> disable if already enabled.
    ClearReg32(GPT_BASE, gptRegOffsets[gpt].ctrlRegOffset, 0x01);

    // The interrupt enable bits for both timers are in the same register. Therefore,
    // block timer ISRs to prevent an ISR from enabling a timer which is then disabled
    // because this function writes a zero to that bit in the IER register.

    uint32_t prevBasePri = BlockIrqs();
    // GPT_IER[gpt] = 1 -> enable interrupt.
    SetReg32(GPT_BASE, 0x04, mask);
    RestoreIrqs(prevBasePri);

    // GPTx_ICNT = delay in milliseconds (assuming 1KHz clock in GPTx_CTRL).
    // Note 1KHz is approximate - the precise value depends on the clock source,
    // but it will be 0.99kHz to 2 decimal places.
    WriteReg32(GPT_BASE, gptRegOffsets[gpt].icntRegOffset, periodMs);

    // GPTx_CTRL -> auto clear; 1kHz, one shot, enable timer.
    WriteReg32(GPT_BASE, gptRegOffsets[gpt].ctrlRegOffset, 0x9);
}
