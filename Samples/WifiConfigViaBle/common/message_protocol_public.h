/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/// <summary>Specifies the type of a message protocol message category ID.</summary>
typedef uint16_t MessageProtocol_CategoryId;

/// <summary>Specifies the type of a message protocol request ID.</summary>
typedef uint16_t MessageProtocol_RequestId;

/// <summary>Specifies the type of a message protocol event ID.</summary>
typedef uint16_t MessageProtocol_EventId;

/// <summary>Specifies the type of a message protocol response result.</summary>
typedef uint8_t MessageProtocol_ResponseResult;

/// <summary>Category ID for a BLE control message.</summary>
static const MessageProtocol_CategoryId MessageProtocol_BleControlCategoryId = 0x0001;

/// <summary>Category ID for a Wi-Fi configuration message.</summary>
static const MessageProtocol_CategoryId MessageProtocol_WifiConfigCategoryId = 0x0002;
