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
#pragma once
#include <stdlib.h>
#include <inttypes.h>

/**@brief Function for sending data via UART.
 *
 * @details This function will send data to the UART module.
 *
 * @param[in] p_data_to_send       The data to send.
 * @param[in] total_bytes_to_send  The size of the data in bytes.
 */
void send_data_via_uart(uint8_t const *p_data_to_send, uint32_t total_bytes_to_send);

/**@brief  Function signature for a callback handler for received UART data.
 *
 * @param[in] p_received_data         The received data.
 * @param[in] p_received_data_length  The size of the data in bytes.
 */
typedef void (*received_uart_data_handler_t)(uint8_t *p_received_data, uint8_t *p_received_data_length);

/**@brief  Function for initializing the UART module.
 *
 * @param[in] received_uart_data_handler  The handler for received UART data.
 */
/**@snippet [UART Initialization] */
void uart_init(received_uart_data_handler_t received_uart_data_handler);
