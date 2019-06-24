#ifndef CUSTOM_BOARD_CONFIG_H
#define CUSTOM_BOARD_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER     0 

#define BUTTONS_NUMBER  0 
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0

#define RX_PIN_NUMBER  28 
#define TX_PIN_NUMBER  26 
#define CTS_PIN_NUMBER 27 
#define RTS_PIN_NUMBER 25 
#define HWFC           true

#define SER_CONN_CHIP_RESET_PIN   21 // Pin used to reset connectivity chip

#ifdef __cplusplus
}
#endif

#endif //CUSTOM_BOARD_CONFIG_H