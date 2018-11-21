/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"

typedef void (*BleControlMessageProtocol_AdvertisingStartedHandlerType)(void);

/// <summary>
///     Initialize the BLE control message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
/// <param name="handler">A callback handler for the "advertising started" event.</param>
void BleControlMessageProtocol_Init(
    BleControlMessageProtocol_AdvertisingStartedHandlerType handler);

/// <summary>
///     Clean up the BLE control message protocol callback handlers and internal state.
/// </summary>
void BleControlMessageProtocol_Cleanup(void);
