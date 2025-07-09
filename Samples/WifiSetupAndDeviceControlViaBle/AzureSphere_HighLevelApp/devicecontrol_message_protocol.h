/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdbool.h>

/// <summary>
///     Signature for a function to set LED status.
/// <param name="isOn">The LED status to be set to.</param>
/// </summary>
typedef void (*DeviceControlMessageProtocol_SetLedStatusHandlerType)(bool isOn);

/// <summary>
///     Signature for a function to get LED status.
/// <returns>The LED status.</returns>
/// </summary>
typedef bool (*DeviceControlMessageProtocol_GetLedStatusHandlerType)(void);

/// <summary>
///     Initialize the device control message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
void DeviceControlMessageProtocol_Init(
    DeviceControlMessageProtocol_SetLedStatusHandlerType setHandler,
    DeviceControlMessageProtocol_GetLedStatusHandlerType getHandler);

/// <summary>
///     Clean up the device control message protocol callback handlers and internal state.
/// </summary>
void DeviceControlMessageProtocol_Cleanup(void);

/// <summary>
///     Notify remote device about LED status change, this must be called when the change is
///     triggered locally.
/// </summary>
void DeviceControlMessageProtocol_NotifyLedStatusChange(void);
