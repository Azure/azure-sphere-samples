/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "message_protocol.h"
#include "message_protocol_private.h"
#include "message_protocol_utilities.h"
#include "uart_utilities.h"

#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define UART_SEND_BUFFER_SIZE 247u // This is the max MTU size of BLE GATT

#define PROTOCOL_BUSY 1
#define INVALID_REQUEST_DATA 2
#define UART_SEND_MAX_RETRY 20u // Max retry 20 times (2 seconds)

// Message protocol state
typedef enum 
{ 
    IDLE_STATE, 
    SENDING_DATA_STATE
} message_protocol_state_t;

static message_protocol_state_t m_state;
static message_protocol_send_data_to_ble_nus_handler_t m_send_data_to_ble_nus_handler;

// Message protocol request message handlers list
struct RequestHandlerNode {
    MessageProtocol_CategoryId categoryId;
    MessageProtocol_RequestId requestId;
    message_protocol_request_handler_t handler;
    struct RequestHandlerNode *nextNode;
};
static struct RequestHandlerNode *m_request_handler_list;

int message_protocol_send_data_via_uart(uint8_t const *p_data_to_send, uint32_t total_bytes_to_send)
{
    int result = 0;
    uint8_t retry = 0;
    do {
        // Check protocol state
        if (m_state != IDLE_STATE) {
            result = PROTOCOL_BUSY;
            nrf_delay_ms(100);
            retry++;
        } else {
            // Before sending data set state to SENDING_DATA_STATE
            m_state = SENDING_DATA_STATE;
            send_data_via_uart(p_data_to_send, total_bytes_to_send);
            // Data sending over, set back to IDLE_STATE
            m_state = IDLE_STATE;
            result = 0;
        }
    } while (result == PROTOCOL_BUSY && retry < UART_SEND_MAX_RETRY);

    if (result != 0) {
        NRF_LOG_INFO("ERROR: Failed to send UART data, error: %d.\n", result);
    }
    m_state = IDLE_STATE;
    return result;
}

static MessageProtocol_RequestMessage *get_ble_request_message(uint8_t *p_message, uint8_t length)
{
    MessageProtocol_MessageHeader *message_header = (MessageProtocol_MessageHeader *)p_message;
    // Check the message is at least the size of a request header
    if (length < sizeof(MessageProtocol_RequestHeader) ||
        message_header->length + sizeof(MessageProtocol_MessageHeader) <
            sizeof(MessageProtocol_RequestHeader)) {
        NRF_LOG_INFO("ERROR: Received invalid request message - too short.\n");
        return NULL;
    }
    MessageProtocol_RequestMessage *request_message = (MessageProtocol_RequestMessage *)(p_message);
    if (request_message->requestHeader.messageHeaderWithType.type ==
            MessageProtocol_RequestMessageType &&
        request_message->requestHeader.categoryId == MessageProtocol_BleControlCategoryId) {
        return request_message;
    }
    return NULL;
}

static void call_request_handler(MessageProtocol_RequestMessage *p_request_message)
{
    struct RequestHandlerNode *current = m_request_handler_list;
    while (current != NULL) {
        if (current->categoryId == p_request_message->requestHeader.categoryId &&
            current->requestId == p_request_message->requestHeader.requestId) {
            // call handler
            uint16_t data_size = (uint16_t)(
                p_request_message->requestHeader.messageHeaderWithType.messageHeader.length +
                sizeof(MessageProtocol_MessageHeader) - sizeof(MessageProtocol_RequestHeader));
            current->handler(p_request_message->data, data_size,
                             p_request_message->requestHeader.sequenceNumber);
            return;
        }
        current = current->nextNode;
    }
    NRF_LOG_INFO(
        "ERROR: Received request message with unknown Category ID and Request ID: 0x%x, 0x%x.\n",
        p_request_message->requestHeader.categoryId, p_request_message->requestHeader.requestId);
}

void received_uart_data_handler(uint8_t *p_received_data, uint8_t *p_received_data_length)
{
    // Only send UART data over BLE when the message is complete
    if (MessageProtocol_IsMessageComplete(p_received_data, *p_received_data_length)) {
        MessageProtocol_RequestMessage *request_message =
            get_ble_request_message(p_received_data, *p_received_data_length);
        // If request_message isn't NULL, we have received a valid BLE request, handle the
        // request.
        if (request_message != NULL) {
            NRF_LOG_INFO("Handle BLE control request message");
            call_request_handler(request_message);
        } else {
            NRF_LOG_DEBUG("Ready to send data over BLE NUS");
            NRF_LOG_HEXDUMP_DEBUG(p_received_data, *p_received_data_length);

            uint32_t err_code;
            do {
                NRF_LOG_DEBUG("Forward received UART data over BLE NUS");
                uint16_t length = (uint16_t)*p_received_data_length;
                // Send received UART data over BLE NUS
                err_code = m_send_data_to_ble_nus_handler(p_received_data, length);
                if ((err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_BUSY) &&
                    (err_code != NRF_ERROR_NOT_FOUND)) {
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        *p_received_data_length = 0;
    }
}

void message_protocol_send_response(MessageProtocol_CategoryId category_id,
                                    MessageProtocol_RequestId request_id, uint16_t sequence_number,
                                    const uint8_t *p_data, size_t data_size,
                                    MessageProtocol_ResponseResult response_result)
{
    MessageProtocol_ResponseMessage response_message;
    memset(&response_message, 0, sizeof(response_message));

    uint16_t total_message_length = (uint16_t)(data_size + sizeof(MessageProtocol_RequestHeader));
    if (total_message_length > UART_SEND_BUFFER_SIZE) {
        NRF_LOG_INFO("ERROR: Invalid response message - too long: %d.\n", total_message_length);
        return;
    }

    memcpy(response_message.responseHeader.messageHeaderWithType.messageHeader.preamble,
           MessageProtocol_MessagePreamble, sizeof(MessageProtocol_MessagePreamble));
    response_message.responseHeader.messageHeaderWithType.messageHeader.length =
        sizeof(MessageProtocol_ResponseHeader) - sizeof(MessageProtocol_MessageHeader) + data_size;
    response_message.responseHeader.messageHeaderWithType.type =
        MessageProtocol_ResponseMessageType;
    response_message.responseHeader.messageHeaderWithType.reserved = 0x00;
    response_message.responseHeader.categoryId = category_id;
    response_message.responseHeader.requestId = request_id;
    response_message.responseHeader.sequenceNumber = sequence_number;
    response_message.responseHeader.reserved = 0x00;
    response_message.responseHeader.responseResult = response_result;
    if (p_data != NULL && data_size > 0) {
        memcpy(response_message.data, p_data, data_size);
    }

    message_protocol_send_data_via_uart((uint8_t *)(&response_message), total_message_length);
}

void message_protocol_send_event(MessageProtocol_CategoryId category_id,
                                 MessageProtocol_EventId event_id)
{
    MessageProtocol_EventMessage event_message;
    memset(&event_message, 0, sizeof(event_message));
    memcpy(event_message.messageHeaderWithType.messageHeader.preamble,
           MessageProtocol_MessagePreamble, sizeof(MessageProtocol_MessagePreamble));
    event_message.messageHeaderWithType.messageHeader.length =
        sizeof(event_message) - sizeof(MessageProtocol_MessageHeader);
    event_message.messageHeaderWithType.type = MessageProtocol_EventMessageType;
    event_message.messageHeaderWithType.reserved = 0x00;
    event_message.eventInfo.categoryId = category_id;
    event_message.eventInfo.eventId = event_id;

    message_protocol_send_data_via_uart((uint8_t *)(&event_message), sizeof(event_message));
}

void message_protocol_register_request_handler(MessageProtocol_CategoryId category_id,
                                               MessageProtocol_RequestId request_id,
                                               message_protocol_request_handler_t handler)
{
    struct RequestHandlerNode *node = malloc(sizeof(struct RequestHandlerNode));
    node->categoryId = category_id;
    node->requestId = request_id;
    node->handler = handler;
    // Add it to the head of linked list
    node->nextNode = m_request_handler_list;
    m_request_handler_list = node;
}

void message_protocol_init(
    message_protocol_send_data_to_ble_nus_handler_t send_data_to_ble_nus_handler)
{
    m_state = IDLE_STATE;
    m_send_data_to_ble_nus_handler = send_data_to_ble_nus_handler;
    uart_init(received_uart_data_handler);
    m_request_handler_list = NULL;
}

void message_protocol_clean_up(void)
{
    // Free all request handler in the list
    struct RequestHandlerNode *current_request_handler = NULL;
    while (m_request_handler_list != NULL) {
        current_request_handler = m_request_handler_list;
        m_request_handler_list = m_request_handler_list->nextNode;
        free(current_request_handler);
    }
}
