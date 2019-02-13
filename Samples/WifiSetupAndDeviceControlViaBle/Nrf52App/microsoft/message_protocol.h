/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

/// <summary>
///     Send data via UART.
/// </summary>
/// <param name="p_data_to_send">The data to send.</param>
/// <param name="total_bytes_to_send">The size of the data in bytes.</param>
/// <returns>0 if the data was sent successfully, any other value indicates an error occurred.</returns>
int message_protocol_send_data_via_uart(uint8_t const *p_data_to_send,
                                        uint32_t total_bytes_to_send);

/// <summary>
///     Function signature for a callback handler for incoming message protocol request messages.
/// </summary>
/// <param name="p_data">The message parameter data.</param>
/// <param name="data_size">Size of the message parameter data in bytes.</param>
/// <param name="sequence_number">The message sequence number to be included in any response.</param>
typedef void (*message_protocol_request_handler_t)(uint8_t *p_data, uint16_t data_size,
                                                   uint16_t sequence_number);

/// <summary>
///     Register a callback handler for incoming message protocol request messages.
/// </summary>
/// <param name="category_id">The message protocol category ID.</param>
/// <param name="event_id">The message protocol event ID.</param>
/// <param name="handler">The callback handler to register.</param>
void message_protocol_register_request_handler(MessageProtocol_CategoryId category_id,
                                               MessageProtocol_EventId event_id,
                                               message_protocol_request_handler_t handler);

/// <summary>
///     Send a response using the message protocol.
/// </summary>
/// <param name="category_id">The message protocol category ID.</param>
/// <param name="request_id">The message protocol request ID.</param>
/// <param name="sequence_number">The sequence number for this response message.</param>
/// <param name="p_data">The data to add to the response message.</param>
/// <param name="data_size">The size of the data in bytes.</param>
/// <param name="response_result">The message protocol response result.</param>
void message_protocol_send_response(MessageProtocol_CategoryId category_id,
                                    MessageProtocol_RequestId request_id,
                                    uint16_t sequence_number,
                                    const uint8_t *p_data,
                                    size_t data_size,
                                    MessageProtocol_ResponseResult response_result);

/// <summary>
///     Send an event using the message protocol.
/// </summary>
/// <param name="category_id">The message protocol category ID.</param>
/// <param name="event_id">The message protocol event ID.</param>
void message_protocol_send_event(MessageProtocol_CategoryId category_id,
                                 MessageProtocol_EventId event_id);

typedef uint32_t (*message_protocol_send_data_to_ble_nus_handler_t)(uint8_t *p_data,
                                                                    uint16_t length);
/// <summary>
///     Initialize the message protocol callback handlers and UART.
/// </summary>
/// <param name="send_data_to_ble_nus_func">
///     A function that will forward incoming data to the BLE characteristic as required.
/// </param>
void message_protocol_init(
    message_protocol_send_data_to_ble_nus_handler_t send_data_to_ble_nus_func);

/// <summary>
///     Clean up the message protocol callback handlers.
/// </summary>
void message_protocol_clean_up(void);
