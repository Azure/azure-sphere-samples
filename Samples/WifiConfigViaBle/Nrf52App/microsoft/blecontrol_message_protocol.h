/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"
#include "blecontrol_message_protocol_defs.h"

#include <stdbool.h>

/// <summary>
///     Signature for a BLE device initializer function.
/// </summary>
/// <param name="p_name">Device name.</param>
/// <param name="length">Length of the device name, in bytes.</param>
typedef int (*message_protocol_init_ble_device_handler_t)(const uint8_t *p_name, uint8_t length);

/// <summary>
///     Initialize the BLE control message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
/// <param name="init_ble_device_handler">A callback handler that will initialize the BLE stack.</param>
void ble_control_message_protocol_init(
    message_protocol_init_ble_device_handler_t init_ble_device_handler);

/// <summary>
///     Clean up the BLE control message protocol callback handlers and internal state.
/// </summary>
void ble_control_message_protocol_clean_up(void);

/// <summary>
///     Send an event indicating that the device is up and ready for the BLE stack to be initialized.
/// </summary>
void ble_control_message_protocol_send_device_up_event(void);
