/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"
#include <time.h>

/// <summary>
///     BLE states.
/// </summary>
typedef enum {
    /// <summary>
    ///     The attached BLE device has not been initialized.
    /// </summary>
    BleControlMessageProtocolState_Uninitialized,
    /// <summary>
    ///     The attached BLE device is advertising to bonded devices.
    /// </summary>
    BleControlMessageProtocolState_AdvertiseToBondedDevices,
    /// <summary>
    ///     The attached BLE device is advertising to all devices.
    /// </summary>
    BleControlMessageProtocolState_AdvertisingToAllDevices,
    /// <summary>
    ///     The attached BLE device is connected to a BLE central device. The device is not
    ///     advertising while in this state.
    /// </summary>
    BleControlMessageProtocolState_DeviceConnected,
    /// <summary>
    ///     The attached BLE device runs into error.
    /// </summary>
    BleControlMessageProtocolState_Error
} BleControlMessageProtocolState;

/// <summary>
///     Signature for a function to handle BLE state change.
/// <param name="state">The BLE state to be handled.</param>
/// </summary>
typedef void (*BleControlMessageProtocol_StateChangeHandlerType)(
    BleControlMessageProtocolState state);

/// <summary>
///     Initialize the BLE control message protocol by registering callback handlers and setting up
///     internal state.
/// </summary>
/// <param name="handler">A callback handler for the BLE event.</param>
/// <param name="epollFd">epoll file descriptor to use for event polling.</param>
int BleControlMessageProtocol_Init(BleControlMessageProtocol_StateChangeHandlerType handler,
                                   int epollFd);

/// <summary>
///     Clean up the BLE control message protocol callback handlers and internal state.
/// </summary>
void BleControlMessageProtocol_Cleanup(void);

/// <summary>
///     Allow BLE advertising to all devices to enable bonding with new device for a specified
///     length of time. If a remote BLE device is bonded within the time or the time has lapsed
///     without any new bond, BLE device to switch back to advertising to only bonded devices.
/// </summary>
/// <param name="timeout">The length of time of advertising for all devices.</param>
/// <returns>0 if succeeded, -1 if device isn't ready yet.</returns>
int BleControlMessageProtocol_AllowNewBleBond(struct timespec *timeout);

/// <summary>
///     Delete all existing bonds on BLE device.
/// </summary>
/// <returns>0 if succeeded, -1 if device isn't ready yet.</returns>
int BleControlMessageProtocol_DeleteAllBondedDevices(void);