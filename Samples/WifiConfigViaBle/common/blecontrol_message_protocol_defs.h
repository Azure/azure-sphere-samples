/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <inttypes.h>

/// <summary>Request ID for Initialize Device Request message.</summary>
static const MessageProtocol_RequestId BleControlMessageProtocol_InitializeDeviceRequestId = 0x0001;

/// <summary>Event ID for a message indicating the attached BLE device has come up.</summary>
static const MessageProtocol_EventId BleControlMessageProtocol_BleDeviceUpEventId = 0x0001;

/// <summary>
///     Data structure for the body of the 
///     <see cref="BleControlMessageProtocol_InitializeDeviceRequestId" /> request message.
/// </summary>
typedef struct {
    /// <summary>Length of the device name.</summary>
    uint8_t deviceNameLength;
    /// <summary>Reserved - must be 0.</summary>
    uint8_t reserved1[3];
    /// <summary>
    ///     Device name - a UTF8-encoded, NULL-terminated string. May be no more than 32
    ///     bytes in length, including the NULL terminator.
    /// </summary>
    uint8_t deviceName[31];
    /// <summary>Reserved - must be 0.</summary>
    uint8_t reserved2;
} BleControlMessageProtocol_InitializeBleDeviceStruct;

/// <summary>
///     Data structure for the body of the
///     <see cref="BleControlMessageProtocol_BleDeviceUpEventId" /> event message.
/// </summary>
typedef struct {
    /// <summary>Device status (not used).</summary>
    uint8_t deviceStatus;
    /// <summary>Reserved - must be 0.</summary>
    uint8_t reserved[3];
} BleControlMessageProtocol_BleDeviceStatusStruct;
