/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright© 2020 MikroElektronika d.o.o.
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE. 
 */
/*!
 * \file
 *
 * \brief This file contains API for Load Cell 2 Click driver.
 *
 * \addtogroup loadcell2 Load Cell 2 Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef LOADCELL2_H
#define LOADCELL2_H

#include "drv_digital_in.h"
#include "drv_i2c_master.h"


// -------------------------------------------------------------- PUBLIC MACROS 
/**
 * \defgroup macros Macros
 * \{
 */

/**
 * \defgroup map_mikrobus MikroBUS
 * \{
 */
#define LOADCELL2_MAP_MIKROBUS( cfg, mikrobus ) \
  cfg.scl  = MIKROBUS( mikrobus, MIKROBUS_SCL ); \
  cfg.sda  = MIKROBUS( mikrobus, MIKROBUS_SDA ); \
  cfg.rdy = MIKROBUS( mikrobus, MIKROBUS_INT ) 
/** \} */

/**
 * \defgroup error_code Error Code
 * \{
 */
#define LOADCELL2_RETVAL  uint8_t

#define LOADCELL2_OK           0x00
#define LOADCELL2_INIT_ERROR   0xFF
/** \} */
 
/**
 * \defgroup i2c_address I2C slave address
 * \{
 */
#define LOADCELL2_SLAVE_ADDRESS                                    0x2A
/** \} */

/**
 * \defgroup reg Register map
 * \{
 */
#define LOADCELL2_REG_PU_CTRL                                      0x00
#define LOADCELL2_REG_CTRL1                                        0x01
#define LOADCELL2_REG_CTRL2                                        0x02
#define LOADCELL2_REG_OCAL1_B2                                     0x03
#define LOADCELL2_REG_OCAL1_B1                                     0x04
#define LOADCELL2_REG_OCAL1_B0                                     0x05
#define LOADCELL2_REG_GCAL1_B3                                     0x06
#define LOADCELL2_REG_GCAL1_B2                                     0x07
#define LOADCELL2_REG_GCAL1_B1                                     0x08
#define LOADCELL2_REG_GCAL1_B0                                     0x09
#define LOADCELL2_REG_OCAL2_B2                                     0x0A
#define LOADCELL2_REG_OCAL2_B1                                     0x0B
#define LOADCELL2_REG_OCAL2_B0                                     0x0C
#define LOADCELL2_REG_GCAL2_B3                                     0x0D
#define LOADCELL2_REG_GCAL2_B2                                     0x0E
#define LOADCELL2_REG_GCAL2_B0                                     0x0F
#define LOADCELL2_REG_GCAL2_B1                                     0x10
#define LOADCELL2_REG_I2C_CTRL                                     0x11
#define LOADCELL2_REG_ADC_B2                                       0x12
#define LOADCELL2_REG_ADC_B1                                       0x13
#define LOADCELL2_REG_ADC_B0                                       0x14
#define LOADCELL2_REG_ADC_REG                                      0x15
#define LOADCELL2_REG_OTP_B1                                       0x16
#define LOADCELL2_REG_OTP_B0                                       0x17
#define LOADCELL2_REG_RES_00                                       0x18
#define LOADCELL2_REG_RES_01                                       0x19
#define LOADCELL2_REG_RES_02                                       0x1A
#define LOADCELL2_REG_PGA_REG                                      0x1B
#define LOADCELL2_REG_POW_CTRL                                     0x1C
#define LOADCELL2_REG_RES_03                                       0x1D
#define LOADCELL2_REG_RES_04                                       0x1E
#define LOADCELL2_REG_REV_ID                                       0x1F
/** \} */

/**
 * \defgroup check_error Check error
 * \{
 */
#define LOADCELL2_ERROR                                            0x00
#define LOADCELL2_SUCCESS                                          0x01
/** \} */

/**
 * \defgroup pwr_control Powerup Control ( LOADCELL2_REG_PU_CTRL )
 * \{
 */
#define LOADCELL2_AVDDS                                            0x80
#define LOADCELL2_OSCS                                             0x40
#define LOADCELL2_CR                                               0x20
#define LOADCELL2_CS                                               0x10
#define LOADCELL2_PUR                                              0x08
#define LOADCELL2_PUA                                              0x04
#define LOADCELL2_PUD                                              0x02
#define LOADCELL2_RR                                               0x01
/** \} */

/**
 * \defgroup con1_cfg Control 1 ( LOADCELL2_REG_CTRL1 )
 * \{
 */
 
/**
 * \defgroup gain_val Gain select value
 * \{
 */
#define LOADCELL2_GAIN_VAL_x128                                    128
#define LOADCELL2_GAIN_VAL_x64                                     64
#define LOADCELL2_GAIN_VAL_x32                                     32
#define LOADCELL2_GAIN_VAL_x16                                     16
#define LOADCELL2_GAIN_VAL_x8                                      8
#define LOADCELL2_GAIN_VAL_x4                                      4
#define LOADCELL2_GAIN_VAL_x2                                      2
#define LOADCELL2_GAIN_VAL_x1                                      1
/** \} */

/** \} */


/**
 * \defgroup con2_cfg Control 2 ( LOADCELL2_REG_CTRL2 )
 * \{
 */
 
/**
 * \defgroup rate_sps Conversion rate select
 * \{
 */
#define LOADCELL2_CONV_RATE_SPS_320                                320
#define LOADCELL2_CONV_RATE_SPS_80                                 80
#define LOADCELL2_CONV_RATE_SPS_40                                 40
#define LOADCELL2_CONV_RATE_SPS_20                                 20
#define LOADCELL2_CONV_RATE_SPS_10                                 10
/** \} */

/** \} */

/**
 * \defgroup check_data Data status
 * \{
 */
#define LOADCELL2_DATA_NO_DATA                                     0
#define LOADCELL2_DATA_OK                                          1
/** \} */

/**
 * \defgroup result Result status
 * \{
 */
#define LOADCELL2_GET_RESULT_ERROR                                 0
#define LOADCELL2_GET_RESULT_OK                                    1
/** \} */

/**
 * \defgroup cal_val Etalon weight value
 * \{
 */
#define LOADCELL2_WEIGHT_100G                                      100
#define LOADCELL2_WEIGHT_500G                                      500
#define LOADCELL2_WEIGHT_1000G                                     1000
#define LOADCELL2_WEIGHT_5000G                                     5000
#define LOADCELL2_WEIGHT_10000G                                    10000
/** \} */
 
/**
 * \defgroup def_coeff Default coefficient
 * \{
 */
#define LOADCELL2_DEFAULT_WEIGHT_SCALE_COEFFICIENT                 0.088495575221
/** \} */

/** \} */ // End group macro 
// --------------------------------------------------------------- PUBLIC TYPES
/**
 * \defgroup type Types
 * \{
 */

/**
 * @brief Click ctx object definition.
 */
typedef struct
{
   //digital_out_t rdy;

    // Input pins 

    digital_in_t rdy;
    
    // Modules 

    i2c_master_t i2c;

    // ctx variable 

    uint8_t slave_address;

} loadcell2_t;

/**
 * @brief Click configuration structure definition.
 */
typedef struct
{
    // Communication gpio pins 

    pin_name_t scl;
    pin_name_t sda;
    
    // Additional gpio pins 

    pin_name_t rdy;

    // static variable 

    uint32_t i2c_speed;
    uint8_t i2c_address;

} loadcell2_cfg_t;

/**
 * @brief Click data structure definition.
 */
typedef struct
{
    float tare;
    uint8_t tare_ok;
    float weight_coeff_100g;
    uint8_t weight_data_100g_ok;
    float weight_coeff_500g;
    uint8_t weight_data_500g_ok;
    float weight_coeff_1000g;
    uint8_t weight_data_1000g_ok;
    float weight_coeff_5000g;
    uint8_t weight_data_5000g_ok;
    float weight_coeff_10000g;
    uint8_t weight_data_10000g_ok;
}
loadcell2_data_t;

/** \} */ // End types group
// ----------------------------------------------- PUBLIC FUNCTION DECLARATIONS

/**
 * \defgroup public_function Public function
 * \{
 */
 
#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Config Object Initialization function.
 *
 * @param cfg  Click configuration structure.
 *
 * @description This function initializes click configuration structure to init state.
 * @note All used pins will be set to unconnected state.
 */
void loadcell2_cfg_setup ( loadcell2_cfg_t *cfg );

/**
 * @brief Initialization function.
 *
 * @param ctx Click object.
 * @param cfg Click configuration structure.
 * 
 * @description This function initializes all necessary pins and peripherals used for this click.
 */
LOADCELL2_RETVAL loadcell2_init ( loadcell2_t *ctx, loadcell2_cfg_t *cfg );

/**
 * @brief Click Default Configuration function.
 *
 * @param ctx  Click object.
 *
 * @description This function executes default configuration for LoadCell2 click.
 * 
 * @note
 * - set LDO Voltage of 3.3V and internal LDO
 * - set gain x128
 * - set conversion rate of 80 SPS
 * - turn Off clock frequency
 * - enables PGA output bypass capacitor
 */
void loadcell2_default_cfg ( loadcell2_t *ctx );

/**
 * @brief Generic write function.
 *
 * @param ctx          Click object.
 * @param reg          Register address.
 * @param data_buf     Data buf to be written.
 * @param len          Number of the bytes in data buf.
 *
 * @description This function writes data to the desired register.
 */
void loadcell2_generic_write ( loadcell2_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len );

/**
 * @brief Generic read function.
 *
 * @param ctx          Click object.
 * @param reg          Register address.
 * @param data_buf     Output data buf
 * @param len          Number of the bytes to be read
 *
 * @description This function reads data from the desired register.
 */
void loadcell2_generic_read ( loadcell2_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len );

/**
 * @brief Check data ready function
 *
 * @param loadcell2 Click object.
 * 
 * @return Data ready:
 * - 0 : Not ready;
 * - 1 : New data ready;
 *
 * @description The function check measurement data ready
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
uint8_t loadcell2_check_data_ready ( loadcell2_t *ctx );

/**
 * @brief Reset function
 *
 * @param loadcell2 Click object.
 * 
 * @description The function resets all registers of NAU7802 24-Bit Dual-Channel ADC
 * on Load Cell 2 Click board.
 */
void loadcell2_reset ( loadcell2_t *ctx );

/**
 * @brief Power On function
 * 
 * @param loadcell2 Click object.
 *
 * @return
 * - 0 : Error;
 * - 1 : Success;
 *
 * @description The function set power up analog and digital circuit
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
LOADCELL2_RETVAL loadcell2_power_on ( loadcell2_t *ctx );

/**
 * @brief Set LDO Voltage function
 * 
 * @param loadcell2 Click object.
 *
 * @description The function set LDO Voltage of 3.3V and internal LDO
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
void loadcell2_set_ldo_voltage ( loadcell2_t *ctx );

/**
 * @brief Set gain function
 *
 * @param loadcell2 Click object.
 * @param gain_val    Gain select :
 *                                 - 128 : LOADCELL2_GAIN_VAL_x128;
 *                                 -  64 : LOADCELL2_GAIN_VAL_x64;
 *                                 -  32 : LOADCELL2_GAIN_VAL_x32;
 *                                 -  16 : LOADCELL2_GAIN_VAL_x16;
 *                                 -   8 : LOADCELL2_GAIN_VAL_x8;
 *                                 -   4 : LOADCELL2_GAIN_VAL_x4;
 *                                 -   2 : LOADCELL2_GAIN_VAL_x2;
 *                                 -   1 : LOADCELL2_GAIN_VAL_x1;
 *
 * @description  The function set gain of NAU7802 24-Bit Dual-Channel ADC
 * on Load Cell 2 Click board.
 */
void loadcell2_set_gain ( loadcell2_t *ctx, uint8_t gain_val );

/**
 * @brief Set sample rate function
 *
 * @param loadcell2 Click object.
 * @param rate_sps    Conversion rate select :
 *                                 - 320 : LOADCELL2_CONV_RATE_SPS_320;
 *                                 -  80 : LOADCELL2_CONV_RATE_SPS_80;
 *                                 -  40 : LOADCELL2_CONV_RATE_SPS_40;
 *                                 -  20 : LOADCELL2_CONV_RATE_SPS_20;
 *                                 -  10 : LOADCELL2_CONV_RATE_SPS_10;
 *
 * @description The function set sample rate of NAU7802 24-Bit Dual-Channel ADC
 * on Load Cell 2 Click board.
 */
void loadcell2_set_sample_rate ( loadcell2_t *ctx, uint8_t rate_sps );

/**
 * @brief Turn Off clk chp function
 * 
 * @param loadcell2 Click object.
 *
 * @description The function turn Off selected the CLK_CHP clock frequency
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
void loadcell2_turn_off_clk_chp ( loadcell2_t *ctx );

/**
 * @brief Enables PGA output function
 * 
 * @param loadcell2 Click object.
 *
 * @description The function enables PGA output bypass capacitor
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
void loadcell2_enable_dec_cap ( loadcell2_t *ctx );

/**
 * @brief Calibrate analog front end of system function.
 * 
 * @param loadcell2 Click object.
 * 
 * @description The function enables PGA output bypass capacitor
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 *
 * @note
 * It is recommended that the AFE be re-calibrated 
 * any time the gain, SPS, or channel number is changed.
 */
void loadcell2_calibrate_afe ( loadcell2_t *ctx );

/**
 * @brief Get results function.
 * 
 * @param loadcell2 Click object.
 * 
 * @description The function read ADC conversion result 
 * of NAU7802 24-Bit Dual-Channel ADC on Load Cell 2 Click board.
 */
uint32_t loadcell2_get_result ( loadcell2_t *ctx );

/**
 * @brief Tare function.
 * 
 * @param loadcell2 Click object.
 * 
 * @description This function tare load cell.
 */
void loadcell2_tare ( loadcell2_t *ctx, loadcell2_data_t *cell_data );

/**
 * @brief Calibration function.
 * 
 * @param loadcell2 Click object.
 * 
 * @description This function do calibration.
 */
uint8_t loadcell2_calibration ( loadcell2_t *ctx, uint16_t cal_val, loadcell2_data_t *cell_data );

/**
 * @brief Get weight function.
 * 
 * @param loadcell2 Click object.
 * 
 * @description This function messure weight.
 */
float loadcell2_get_weight ( loadcell2_t *ctx, loadcell2_data_t *cell_data );

/**
 * @brief Check data ready function
 * 
 * @param loadcell2 Click object.
 *
 * @return
 * - 0 : Data Not Ready;
 * - 1 : Data Ready;
 *
 * @description The function check data ready state by return state
 * of the INT pin of Load Cell 2 Click board.
*/
uint8_t loadcell2_check_drdy ( loadcell2_t *ctx );



#ifdef __cplusplus
}
#endif
#endif  // _LOADCELL2_H_

/** \} */ // End public_function group
/// \}    // End click Driver group  
/*! @} */
// ------------------------------------------------------------------------- END
