#ifndef __LL_H
#define __LL_H

#include <stdint.h>

extern int ll_gpio_init(void);
extern void ll_gpio_cs_go_low(void);
extern void ll_gpio_cs_go_high(void);

extern int ll_i2c_init(void);
extern int ll_i2c_tx(uint8_t* tx_data, uint32_t tx_len);
extern int ll_i2c_tx_then_rx(uint8_t *tx_data, uint32_t tx_len, uint8_t *rx_data, uint32_t rx_len);

extern int ll_spi_init(void);
extern int ll_spi_tx(uint8_t* tx_data, uint32_t tx_len);
extern int ll_spi_rx(uint8_t* rx_data, uint32_t rx_len);
extern int ll_spi_tx_then_rx(uint8_t* tx_data, uint32_t tx_len, uint8_t* rx_data, uint32_t rx_len);

#endif