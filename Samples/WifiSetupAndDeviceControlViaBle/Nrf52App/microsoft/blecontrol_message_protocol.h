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
/// <returns>0 if initialization succeeded, other values if an error occurred.</returns>
typedef int (*message_protocol_init_ble_device_handler_t)(const uint8_t *p_name, uint8_t length);
/// <summary>
///     Signature for a function to set BLE passkey.
/// </summary>
/// <param name="p_passkey">BLE passkey, which should contain six digit number (from "000000" to "999999").</param>
/// <returns>0 if set passkey succeeded, other values if an error occurred.</returns>
typedef int (*message_protocol_set_passkey_handler_t)(const uint8_t *p_passkey);
/// <summary>
///     Signature for a function to start BLE advertising.
/// </summary>
/// <param name="use_whitelist">Whether to use whitelist for BLE advertising.</param>
/// <returns>0 if advertising started successfully, other values if an error occurred.</returns>
typedef int (*message_protocol_start_advertising_handler)(bool use_whitelist);
/// <summary>
///     Signature for a function to delete all BLE bonds.
/// </summary>
/// <returns>0 if deleting all BLE bonds successfully, other values if an error occurred.</returns>
typedef int (*message_protocol_delete_all_bonds_handler)(void);

/// <summary>
///     Initialize the BLE control message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
/// <param name="init_ble_device_handler">A callback handler that will initialize the BLE stack.</param>
/// <param name="set_passkey_handler">A callback handler that will set BLE passkey.</param>
void ble_control_message_protocol_init(
    message_protocol_init_ble_device_handler_t init_ble_device_handler,
    message_protocol_set_passkey_handler_t set_passkey_handler,
    message_protocol_start_advertising_handler start_advertising_handler,
    message_protocol_delete_all_bonds_handler delete_all_bonds_handler);

/// <summary>
///     Clean up the BLE control message protocol callback handlers and internal state.
/// </summary>
void ble_control_message_protocol_clean_up(void);

/// <summary>
///     Send an event indicating that the device is up and ready for the BLE stack to be initialized.
/// </summary>
void ble_control_message_protocol_send_device_up_event(void);

/// <summary>
///     Send an event to indicate the device just connected to a BLE central device.
/// </summary>
void ble_control_message_protocol_send_connected_event(void);

/// <summary>
///     Send an event indicate the device just disconnected from a BLE central device.
/// </summary>
void ble_control_message_protocol_send_disconnected_event(void);

/// <summary>
///     Send an event indicate the need to display passkey during BLE pairing process.
/// </summary>
void ble_control_message_protocol_send_display_passkey_needed_event(void);
