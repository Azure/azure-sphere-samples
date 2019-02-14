/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>

/// <summary>
///     Request ID for Initialize Device Request message.
///     This message must be sent before any other BLE request.
/// </summary>
static const MessageProtocol_RequestId BleControlMessageProtocol_InitializeDeviceRequestId = 0x0001;
/// <summary>
///     Request ID for Set Passkey Request message. This message can be sent multiple times.
///     The BLE device will use the passkey from the last message for pairing.
/// </summary>
static const MessageProtocol_RequestId BleControlMessageProtocol_SetPasskeyRequestId = 0x0002;
/// <summary>
///     Request ID for Change BLE Advertising Mode Request message.
/// </summary>
static const MessageProtocol_RequestId BleControlMessageProtocol_ChangeBleAdvertisingModeRequestId =
    0x0003;
/// <summary>
///     Request ID for Delete All BLE Bonds Request message.
/// </summary>
static const MessageProtocol_RequestId BleControlMessageProtocol_DeleteAllBleBondsRequestId =
    0x0004;

/// <summary>Event ID for a message indicating the attached BLE device has come up.</summary>
static const MessageProtocol_EventId BleControlMessageProtocol_BleDeviceUpEventId = 0x0001;
/// <summary>
///     Event ID for a message indicating the attached BLE device has connected to a BLE central
///     device.
/// </summary>
static const MessageProtocol_EventId BleControlMessageProtocol_BleDeviceConnectedEventId = 0x0002;
/// <summary>
///     Event ID for a message indicating the attached BLE device has disconnected from a BLE
///     central device.
/// </summary>
static const MessageProtocol_EventId BleControlMessageProtocol_BleDeviceDisconnectedEventId =
    0x0003;
/// <summary>
///     Event ID for a message indicating the need to display passkey during BLE pairing process.
/// </summary>
static const MessageProtocol_EventId BleControlMessageProtocol_DisplayPasskeyNeededEventId = 0x0004;

/// <summary>
///     Data structure for the body of the
///     <see cref="BleControlMessageProtocol_InitializeDeviceRequestId" /> request message.
/// </summary>
typedef struct {
    /// <summary>Length of the device name.</summary>
    uint8_t deviceNameLength;
    /// <summary>Reserved - must all be 0.</summary>
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
///     <see cref="BleControlMessageProtocol_SetPasskeyRequestId" /> request message.
/// </summary>
typedef struct {
    /// <summary>
    ///     Passkey - 6-digit ASCII string (digit 0..9 only, no NULL termination).
    /// </summary>
    uint8_t passkey[6];
    /// <summary>Reserved - must all be 0.</summary>
    uint8_t reserved[2];
} BleControlMessageProtocol_SetPasskeyStruct;

/// <summary>
///     BLE advertising modes.
/// </summary>
typedef enum {
    /// <summary>BLE not advertising mode.</summary>
    BleControlMessageProtocol_NotAdvertisingMode,
    /// <summary>BLE advertising only to existing bonded devices.</summary>
    BleControlMessageProtocol_AdvertisingToBondedDevicesMode,
    /// <summary>BLE advertising to all devices.</summary>
    BleControlMessageProtocol_AdvertisingToAllMode
} BleControlMessageProtocol_BleAdvertisingMode;

/// <summary>
///     Data structure for the body of the
///     <see cref="BleControlMessageProtocol_ChangeBleAdvertisingModeRequestId" /> request message.
/// </summary>
typedef struct {
    /// <summary>BLE advertising mode.</summary>
    uint8_t mode;
    /// <summary>Reserved - must all be 0.</summary>
    uint8_t reserved[3];
} BleControlMessageProtocol_ChangeBleAdvertisingModeStruct;
