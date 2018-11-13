/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

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