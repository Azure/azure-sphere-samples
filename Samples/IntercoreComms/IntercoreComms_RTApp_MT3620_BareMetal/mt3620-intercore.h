/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "logical-intercore.h"

/// <summary>
///     Uses the mailbox to get the inbound and outbound buffer bases and stores the
///     supplied callback, to invoke later when a message is received.
/// </summary>
/// <param name="inboundBase">
///     On return, set to a 32-bit value which encodes both
///     the buffer header pointer and the buffer size.
/// </param>
/// <param name="outboundBase">
///     On return, set to a 32-bit value which encodes both
///     the buffer header pointer and the buffer size.
/// </param>
/// <param name="recvCallback">
///     This function will be enqueued as a DPC when an incoming message is received.
///     The application must call <see cref="EnqueueDeferredProc" /> to run it.
/// </param>
void MT3620_SetupIntercoreComm(uint32_t *inboundBase, uint32_t *outboundBase,
                               Callback recvCallback);

/// <summary>
///     Handles interrupt when an incoming message is received. The application should not
///     call this function directly, but should use it in the vector table.
/// </summary>
void MT3620_HandleMailboxIrq11(void);

/// <summary>
///     Raise an interrupt to tell the high-level core that a message has been sent.
/// </summary>
void MT3620_SignalHLCoreMessageSent(void);

/// <summary>
///     Raise an interrupt to tell the high-level core that an incoming message has been
///     received and read.
/// </summary>
void MT3620_SignalHLCoreMessageReceived(void);
