/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "blecontrol_message_protocol.h"
#include "message_protocol.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static message_protocol_init_ble_device_handler_t m_init_ble_device_handler;
static message_protocol_set_passkey_handler_t m_set_passkey_handler;
static message_protocol_start_advertising_handler m_start_advertising_handler;
static message_protocol_delete_all_bonds_handler m_delete_all_bonds_handler;

// BLE control request message handlers
static void ble_control_initialize_device_request_handler(uint8_t *p_data, uint16_t data_size,
                                                          uint16_t sequence_number)
{
    // process request data and send response
    if (data_size != sizeof(BleControlMessageProtocol_InitializeBleDeviceStruct)) {
        NRF_LOG_INFO(
            "INFO: BLE control \"Initialize BLE device\" request message has invalid size: %d.\n",
            data_size);
        return;
    }

    BleControlMessageProtocol_InitializeBleDeviceStruct *init_struct =
        (BleControlMessageProtocol_InitializeBleDeviceStruct *)p_data;
    uint8_t result =
        m_init_ble_device_handler(init_struct->deviceName, init_struct->deviceNameLength);
    message_protocol_send_response(MessageProtocol_BleControlCategoryId,
                                   BleControlMessageProtocol_InitializeDeviceRequestId,
                                   sequence_number, NULL, 0, result);
}

static void ble_control_set_passkey_request_handler(uint8_t *p_data, uint16_t data_size,
                                                          uint16_t sequence_number)
{
    // process request data and send response
    if (data_size != sizeof(BleControlMessageProtocol_SetPasskeyStruct)) {
        NRF_LOG_INFO(
            "INFO: BLE control \"Set Passkey\" request message has invalid size: %d.\n",
            data_size);
        return;
    }

    BleControlMessageProtocol_SetPasskeyStruct *passkey_struct =
        (BleControlMessageProtocol_SetPasskeyStruct *)p_data;
    uint8_t result = m_set_passkey_handler(passkey_struct->passkey);
    message_protocol_send_response(MessageProtocol_BleControlCategoryId,
                                   BleControlMessageProtocol_SetPasskeyRequestId,
                                   sequence_number, NULL, 0, result);
}

static void ble_control_change_ble_mod_request_handler(uint8_t *p_data, uint16_t data_size,
                                                          uint16_t sequence_number)
{
    // process request data and send response
    if (data_size != sizeof(BleControlMessageProtocol_ChangeBleAdvertisingModeStruct)) {
        NRF_LOG_INFO(
            "INFO: BLE control \"Change BLE Mode\" request message has invalid size: %d.\n",
            data_size);
        return;
    }

    BleControlMessageProtocol_ChangeBleAdvertisingModeStruct *mode_struct =
        (BleControlMessageProtocol_ChangeBleAdvertisingModeStruct *)p_data;

    uint8_t result;
    if(mode_struct->mode != BleControlMessageProtocol_AdvertisingToBondedDevicesMode
        && mode_struct->mode != BleControlMessageProtocol_AdvertisingToAllMode)
    {
        NRF_LOG_INFO(
            "ERROR: BLE control \"Change BLE Mode\" request message has invalid mode: %d.\n",
            mode_struct->mode);
        result = 1;
    } else {
        result = m_start_advertising_handler(
            mode_struct->mode == BleControlMessageProtocol_AdvertisingToBondedDevicesMode);
    }
    message_protocol_send_response(MessageProtocol_BleControlCategoryId,
                                   BleControlMessageProtocol_ChangeBleAdvertisingModeRequestId,
                                   sequence_number, p_data, 
                                   sizeof(BleControlMessageProtocol_ChangeBleAdvertisingModeStruct), result);
}

static void ble_control_delete_all_bonds_request_handler(uint8_t *p_data, uint16_t data_size,
                                                          uint16_t sequence_number)
{
    // process request data and send response
    if (data_size != 0) {
        NRF_LOG_INFO(
            "INFO: BLE control \"Delete All BLE Bonds\" request message has invalid size: %d.\n",
            data_size);
        return;
    }

    uint8_t result = m_delete_all_bonds_handler();
    message_protocol_send_response(MessageProtocol_BleControlCategoryId,
                                   BleControlMessageProtocol_DeleteAllBleBondsRequestId,
                                   sequence_number, NULL, 0, result);
}

void ble_control_message_protocol_init(
    message_protocol_init_ble_device_handler_t init_ble_device_handler,
    message_protocol_set_passkey_handler_t set_passkey_handler,
    message_protocol_start_advertising_handler start_advertising_handler,
    message_protocol_delete_all_bonds_handler delete_all_bonds_handler)
{
    m_init_ble_device_handler = init_ble_device_handler;
    m_set_passkey_handler = set_passkey_handler;
    m_start_advertising_handler = start_advertising_handler;
    m_delete_all_bonds_handler = delete_all_bonds_handler;

    message_protocol_register_request_handler(MessageProtocol_BleControlCategoryId,
                                              BleControlMessageProtocol_InitializeDeviceRequestId,
                                              ble_control_initialize_device_request_handler);
    message_protocol_register_request_handler(MessageProtocol_BleControlCategoryId,
                                              BleControlMessageProtocol_SetPasskeyRequestId,
                                              ble_control_set_passkey_request_handler);
    message_protocol_register_request_handler(MessageProtocol_BleControlCategoryId,
                                              BleControlMessageProtocol_ChangeBleAdvertisingModeRequestId,
                                              ble_control_change_ble_mod_request_handler);
    message_protocol_register_request_handler(MessageProtocol_BleControlCategoryId,
                                              BleControlMessageProtocol_DeleteAllBleBondsRequestId,
                                              ble_control_delete_all_bonds_request_handler);
}

void ble_control_message_protocol_clean_up(void) {}

void ble_control_message_protocol_send_device_up_event(void)
{
    message_protocol_send_event(MessageProtocol_BleControlCategoryId,
                                BleControlMessageProtocol_BleDeviceUpEventId);
}

void ble_control_message_protocol_send_connected_event(void)
{
    message_protocol_send_event(MessageProtocol_BleControlCategoryId,
                                BleControlMessageProtocol_BleDeviceConnectedEventId);
}

void ble_control_message_protocol_send_disconnected_event(void)
{
    message_protocol_send_event(MessageProtocol_BleControlCategoryId,
                                BleControlMessageProtocol_BleDeviceDisconnectedEventId);
}

void ble_control_message_protocol_send_display_passkey_needed_event(void)
{
    message_protocol_send_event(MessageProtocol_BleControlCategoryId,
                                BleControlMessageProtocol_DisplayPasskeyNeededEventId);
}