/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"

/// <summary>
///     Initialize the BLE control message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
void BleControlMessageProtocol_Init(void);

/// <summary>
///     Clean up the BLE control message protocol callback handlers and internal state.
/// </summary>
void BleControlMessageProtocol_Cleanup(void);

typedef void (*BleControlMessageProtocol_DeviceUpHandlerType)(void);

/// <summary>
///     Register a callback handler for the "device up" event.
/// </summary>
/// <param name="handler">The callback handler to register.</param>
void BleControlMessageProtocol_RegisterDeviceUpHandler(
    BleControlMessageProtocol_DeviceUpHandlerType handler);

typedef void (*BleControlMessageProtocol_AdvertisingStartedHandlerType)(void);

/// <summary>
///     Initialize the device to use the BLE control message protocol, and register
///     a handler for the "advertising started" event.
/// </summary>
/// <param name="handler">A callback handler for the "advertising started" event.</param>
void BleControlMessageProtocol_InitializeDevice(
    BleControlMessageProtocol_AdvertisingStartedHandlerType handler);
