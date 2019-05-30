/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_TIMER_H
#define MT3620_TIMER_H

#include <stdint.h>

#include "mt3620-baremetal.h"

/// <summary>
/// <para>An instance of this is passed to <see cref="Gpt_LaunchTimerMs" /> to register a
/// callback.</para>
/// <para>Only the interrupt-based timers GTP0 and GTP1 are supported.</para>
/// </summary>
typedef enum {
    /* Identifier for GPT0. */
    TimerGpt0 = 0,
    /* Identifier for GPT1. */
    TimerGpt1 = 1
} TimerGpt;

/// <summary>Total number of supported GPTs.</summary>
#define TIMER_GPT_COUNT 2
/// <summary>The GPT interrupts (and hence callbacks) run at this priority level.</summary>
static const uint32_t GPT_PRIORITY = 2;

/// <summary>
/// Call this once before registering any callbacks with <see cref="Gpt_LaunchTimerMs" />.
/// </summary>
void Gpt_Init(void);

/// <summary>
/// To use the GPT, install this function as the INT1 handler in the exception table.
/// Applications should not call this function directly.
/// </summary>
void Gpt_HandleIrq1(void);

/// <summary>
/// <para>Register a callback for the supplied timer. Only one callback can be registered
/// at a time for each timer. If a callback is already registered, then the timer is
/// cancelled, the new callback is installed, and the timer is restarted. The callback
/// runs in interrupt context.</para>
/// <para>The callback will be invoked once. The callback can re-register itself by calling
/// this function.</para>
/// <para>Only call this function from the main application thread or from a timer callback.</para>
/// <para>The application should install the <see cref="Gpt_HandleIrq1" /> interrupt handler
/// and call <see cref="Gpt_Init" /> before calling this function.</para>
/// </summary>
/// <param name="gpt">Which hardware timer to use.</param>
/// <param name="periodMs">Period in milliseconds.</param>
/// <param name="callback">Function to invoke in interrupt context when the timer expires.</param>
void Gpt_LaunchTimerMs(TimerGpt gpt, uint32_t periodMs, Callback callback);

#endif /* MT3620_TIMER_H */
