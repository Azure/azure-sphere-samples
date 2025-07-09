/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>

#include "mt3620-baremetal.h"

/// <summary>
///     <para>
///         This node is used to build a linked list of deferred procedure calls (DPCs)
///         which can be scheduled with <see cref="EnqueueDeferredProc" /> and invoked with
///         <see cref="InvokeDeferredProcs" />.
///     </para>
///     <para>The application should not modify this object after it has been initialized.</para>
/// </summary>
typedef struct CallbackNode {
    /// <summary>Internal use. Initialize to false.</summary>
    bool enqueued;
    /// <summary>Internal use. Initialize to NULL.</summary>
    struct CallbackNode *next;
    /// <summary>
    ///     Initialize to callback function which is invoked after
    ///     the processor leaves interrupt context.
    /// </summary>
    Callback cb;
} CallbackNode;

/// <summary>
///     This function should be called from an interrupt service routine.
///     It schedules a function to be run when the core leaves IRQ context.
///     The callbacks will be run by <see cref="InvokeDeferredProcs" />.
/// </summary>
/// <param name="node">
///     Contains function to schedule. This object must exist until the deferred
///     function call has completed.
/// </param>
void EnqueueDeferredProc(CallbackNode *node);

/// <summary>
///     Runs any DPCs which have been scheduled with <see cref="EnqueueDeferredProc" />.
///     The RTApp will typically set up its resources and then go into a loop
///     which waits for an interrupt, and then calls this function to schedule
///     enqueued DPCs.
/// </summary>
void InvokeDeferredProcs(void);
