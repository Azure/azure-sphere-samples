/**
 * This code is based on a sample from Nordic Semiconductor ASA (see license below),
 * with modifications made by Microsoft (see the README.md in this directory).
 *
 * Modified version of ble_peripheral\ble_app_uart example from Nordic nRF5 SDK version 15.2.0
 * (https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip)
 *
 * Original file: {SDK_ROOT}\examples\ble_peripheral\ble_app_uart\main.c:uart_init, ble_evt_handler, nus_data_handler
 **/

/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "uart_utilities.h"

#include "ble_nus.h"
#include "app_uart.h"
#include "bsp_btn_ble.h"

#if defined(UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined(UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define UART_TX_BUF_SIZE 256 /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256 /**< UART RX buffer size. */

static received_uart_data_handler_t m_received_uart_data_handler;

/**@brief Function for sending data via UART.
 *
 * @details This function will send data to the UART module.
 *
 * @param[in] p_data_to_send       The data to send.
 * @param[in] total_bytes_to_send  The size of the data in bytes.
 */
void send_data_via_uart(uint8_t const *p_data_to_send, uint32_t total_bytes_to_send)
{
    uint32_t err_code;
    NRF_LOG_INFO("Writing data on UART.");
    for (uint32_t i = 0; i < total_bytes_to_send; i++) {
        do {
            err_code = app_uart_put(p_data_to_send[i]);
            if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY)) {
                NRF_LOG_ERROR("Failed sending UART data. Error 0x%x. ", err_code);
                APP_ERROR_CHECK(err_code);
            }
        } while (err_code == NRF_ERROR_BUSY);
    }
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a buffer. If the event type is APP_UART_DATA_READY, the received UART data handle is
 *          called each time to process the data.
 */
/**@snippet [Handling the data received over UART] */
static void uart_event_handle(app_uart_evt_t *p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;

    switch (p_event->evt_type) {
    case APP_UART_DATA_READY:
        UNUSED_VARIABLE(app_uart_get(&data_array[index]));
        index++;
        m_received_uart_data_handler(data_array, &index);
        break;

    case APP_UART_COMMUNICATION_ERROR:
        APP_ERROR_HANDLER(p_event->data.error_communication);
        break;

    case APP_UART_FIFO_ERROR:
        APP_ERROR_HANDLER(p_event->data.error_code);
        break;

    default:
        break;
    }
}

/**@brief  Function for initializing the UART module.
 *
 * @param[in] received_uart_data_handler  The handler for received UART data.
 */
/**@snippet [UART Initialization] */
void uart_init(received_uart_data_handler_t received_uart_data_handler)
{
    m_received_uart_data_handler = received_uart_data_handler;

    uint32_t err_code;
    app_uart_comm_params_t const comm_params = {
        .rx_pin_no = RX_PIN_NUMBER,
        .tx_pin_no = TX_PIN_NUMBER,
        .rts_pin_no = RTS_PIN_NUMBER,
        .cts_pin_no = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_ENABLED,
        .use_parity = false,
#if defined(UART_PRESENT)
        .baud_rate = NRF_UART_BAUDRATE_115200
#else
        .baud_rate = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST, err_code);
    APP_ERROR_CHECK(err_code);
}