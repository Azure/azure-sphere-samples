/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "blecontrol_message_protocol.h"
#include "message_protocol.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static message_protocol_init_ble_device_handler_t m_init_ble_device_handle;

// BLE control request message handlers
static void ble_control_initialize_device_request_handler(uint8_t *p_data, uint16_t data_size,
                                                          uint16_t sequence_number)
{
    // process request data and send response
    if (data_size != sizeof(BleControlMessageProtocol_InitializeBleDeviceStruct)) {
        NRF_LOG_INFO("INFO: BLE control \"Initialize BLE device\" request message is invalid size: %d.\n",
                     data_size);
        return;
    }

    BleControlMessageProtocol_InitializeBleDeviceStruct *init_struct =
        (BleControlMessageProtocol_InitializeBleDeviceStruct *)p_data;
    uint8_t result =
        m_init_ble_device_handle(init_struct->deviceName, init_struct->deviceNameLength);
    message_protocol_send_response(MessageProtocol_BleControlCategoryId,
                                   BleControlMessageProtocol_InitializeDeviceRequestId,
                                   sequence_number, NULL, 0, result);
}

void ble_control_message_protocol_init(
    message_protocol_init_ble_device_handler_t init_ble_device_handler)
{
    m_init_ble_device_handle = init_ble_device_handler;

    message_protocol_register_request_handler(MessageProtocol_BleControlCategoryId,
                                              BleControlMessageProtocol_InitializeDeviceRequestId,
                                              ble_control_initialize_device_request_handler);
}

void ble_control_message_protocol_clean_up(void) {}

void ble_control_message_protocol_send_device_up_event(void)
{
    message_protocol_send_event(MessageProtocol_BleControlCategoryId,
                                BleControlMessageProtocol_BleDeviceUpEventId);
}
