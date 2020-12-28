/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "mt3620-baremetal.h"
#include "mt3620-timer-poll.h"

static const uintptr_t GPT_BASE = 0x21030000;

void Gpt3_WaitUs(int microseconds)
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
