/**
 * This file is based on a sample from Nordic Semiconductor ASA (see LICENSE.txt in parent directory), 
 * with modifications made by Microsoft (see the README.md in parent directory).
 *
 * Modified version of secure_bootloader\pca10040_uart_debug example from Nordic nRF5 SDK version 15.2.0
 * (https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip)
 *
 * Original file: {SDK_ROOT}\components\boards\pca10040.h
 **/

#ifndef CUSTOM_BOARD_CONFIG_H
#define CUSTOM_BOARD_CONFIG_H

// our custom board configuration is based off this board... 
#include "pca10040.h"

// ... but not using these pins
#undef RX_PIN_NUMBER  // p0.08
#undef TX_PIN_NUMBER  // p0.06
#undef CTS_PIN_NUMBER // p0.07
#undef RTS_PIN_NUMBER // p0.05

// ... using these instead
#define RX_PIN_NUMBER  11 
#define TX_PIN_NUMBER  12 
#define CTS_PIN_NUMBER 23 
#define RTS_PIN_NUMBER 22 

#endif //CUSTOM_BOARD_CONFIG_H