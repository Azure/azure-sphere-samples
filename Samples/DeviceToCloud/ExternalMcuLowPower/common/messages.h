/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <assert.h>
#include "message_protocol_private.h"

/// <summary>
///     All messages for the low-power MCU to Cloud application use a single category
/// </summary>
static const MessageProtocol_CategoryId MessageProtocol_McuToCloud_CategoryId = 0x0001;

/// <summary>Initialize request ID</summary>
static const MessageProtocol_RequestId MessageProtocol_McuToCloud_Init = 0x0001;

/// <summary>RequestTelemetry request ID</summary>
static const MessageProtocol_RequestId MessageProtocol_McuToCloud_RequestTelemetry = 0x0002;

/// <summary>SetLed request ID</summary>
static const MessageProtocol_RequestId MessageProtocol_McuToCloud_SetLed = 0x0003;

/// <summary>
/// Protocol version - increment if any of the structures below are changed.
/// </summary>
static const uint32_t MessageProtocol_McuToCloud_ProtocolVersion = 0x002;

/// <summary>
///     Struct for the body of an Init response
/// </summary>
typedef struct {
    /// <summary>
    ///     Version of the protocol in use.
    /// </summary>
    uint32_t protocolVersion;
} MessageProtocol_McuToCloud_InitStruct;

/// <summary>
///     Struct for the body of a RequestTelemetry response
/// </summary>
typedef struct {
    /// <summary>
    /// Accumulated total number of dispenses made by the machine (since first run)
    /// </summary>
    uint32_t lifetimeTotalDispenses;

    /// <summary>
    /// Accumulated total number of dispenses stocked in the machine (since first run)
    /// </summary>
    uint32_t lifetimeTotalStockedDispenses;

    /// <summary>
    /// Maximum number of dispenses that can be stocked at once
    /// </summary>
    uint32_t capacity;

    /// <summary>
    /// Battery level (volts)
    /// </summary>
    float batteryLevel;
} MessageProtocol_McuToCloud_TelemetryStruct;

/// <summary>
/// Struct for the body of a SetLed request
/// </summary>
typedef struct {
    /// <summary>
    /// Value for the red channel (0x00 means off, any other value means on)
    /// </summary>
    uint8_t red;

    /// <summary>
    /// Value for the green channel (0x00 means off, any other value means on)
    /// </summary>
    uint8_t green;

    /// <summary>
    /// Value for the blue channel (0x00 means off, any other value means on)
    /// </summary>
    uint8_t blue;

    /// <summary>
    /// Reserved - must be set to 0
    /// </summary>
    uint8_t reserved;
} MessageProtocol_McuToCloud_SetLedStruct;

// Checks to make sure the structs fit within the max body size as defined in the message protocol
#define MAX_OF(a, b) (((a) > (b)) ? (a) : (b))

#define MAX_BODY_SIZE                                          \
    MAX_OF(sizeof(MessageProtocol_McuToCloud_TelemetryStruct), \
           sizeof(MessageProtocol_McuToCloud_SetLedStruct))

static_assert(MAX_BODY_SIZE <= MAX_REQUEST_DATA_SIZE,
              "MaxBodySize must be smaller or equal to MAX_REQUEST_DATA_SIZE");

static_assert(MAX_BODY_SIZE <= MAX_RESPONSE_DATA_SIZE,
              "MaxBodySize must be smaller or equal to MAX_RESPONSE_DATA_SIZE");

static_assert(sizeof(MessageProtocol_McuToCloud_TelemetryStruct) <= MAX_BODY_SIZE,
              "MessageProtocol_McuToCloud_TelemetryStruct exceeds MaxBodySize");

static_assert(sizeof(MessageProtocol_McuToCloud_SetLedStruct) <= MAX_BODY_SIZE,
              "MessageProtocol_McuToCloud_TelemetryStruct exceeds SetLedStruct");
