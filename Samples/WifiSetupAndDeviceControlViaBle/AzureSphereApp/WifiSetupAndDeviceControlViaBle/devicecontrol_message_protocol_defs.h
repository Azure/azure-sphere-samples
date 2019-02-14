/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"
#include <stdint.h>

/// <summary>Request ID for a Get Desired LED Status request message.</summary>
static const MessageProtocol_RequestId DeviceControlMessageProtocol_GetDesiredLedStatusRequestId =
    0x0001;

/// <summary>Request ID for a Report LED Status request message.</summary>
static const MessageProtocol_RequestId DeviceControlMessageProtocol_ReportLedStatusRequestId =
    0x0002;

/// <summary>Event ID for a Desired LED Status Available event message.</summary>
static const MessageProtocol_EventId DeviceControlMessageProtocol_DesiredLedStatusAvailableEventId =
    0x0001;

/// <summary>Event ID for a LED Status Needed event message.</summary>
static const MessageProtocol_EventId DeviceControlMessageProtocol_LedStatusNeededEventId = 0x0002;

/// <summary>
///     Data structure for the body of a
///     <see cref="DeviceControlMessageProtocol_GetDesiredLedStatusRequestId" /> response message.
///     This structure describes LED status.
/// </summary>
typedef struct {
    /// <summary>
    ///     LED status, 0x00 for off, 0x01 for on.
    /// </summary>
    uint8_t status;
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved[3];
} DeviceControlMessageProtocol_LedStatusStruct;
