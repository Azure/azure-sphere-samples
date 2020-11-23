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
 */

#include "loadcell2.h"

// ------------------------------------------------------------- PRIVATE MACROS 

/**
 * \defgroup gain Gain bit mask
 * \{
 */
#define LOADCELL2_GAIN_x128                                        0x07
#define LOADCELL2_GAIN_x64                                         0x06
#define LOADCELL2_GAIN_x32                                         0x05
#define LOADCELL2_GAIN_x16                                         0x04
#define LOADCELL2_GAIN_x8                                          0x03
#define LOADCELL2_GAIN_x4                                          0x02
#define LOADCELL2_GAIN_x2                                          0x01
#define LOADCELL2_GAIN_x1                                          0x00
/** \} */

/**
 * \defgroup rate Conversion rate bit mask
 * \{
 */
#define LOADCELL2_BIT_CONV_RATE_SPS_320                            0x70
#define LOADCELL2_BIT_CONV_RATE_SPS_80                             0x30
#define LOADCELL2_BIT_CONV_RATE_SPS_40                             0x20
#define LOADCELL2_BIT_CONV_RATE_SPS_20                             0x10
#define LOADCELL2_BIT_CONV_RATE_SPS_10                             0x00
/** \} */

// ---------------------------------------------- PRIVATE FUNCTION DECLARATIONS 

static void dev_rst_delay( void );

static void dev_measure_delay ( void );

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void loadcell2_cfg_setup ( loadcell2_cfg_t *cfg )
{
    // Communication gpio pins 

    cfg->scl = HAL_PIN_NC;
    cfg->sda = HAL_PIN_NC;
    
    // Additional gpio pins

    cfg->rdy = HAL_PIN_NC;

    cfg->i2c_speed = I2C_MASTER_SPEED_STANDARD; 
    cfg->i2c_address = LOADCELL2_SLAVE_ADDRESS;
}

LOADCELL2_RETVAL loadcell2_init ( loadcell2_t *ctx, loadcell2_cfg_t *cfg )
{
    i2c_master_config_t i2c_cfg;

    i2c_master_configure_default( &i2c_cfg );
    i2c_cfg.speed  = cfg->i2c_speed;
    i2c_cfg.scl    = cfg->scl;
    i2c_cfg.sda    = cfg->sda;

    ctx->slave_address = cfg->i2c_address;

    if ( i2c_master_open( &ctx->i2c, &i2c_cfg ) == I2C_MASTER_ERROR )
    {
        return LOADCELL2_INIT_ERROR;
    }

    i2c_master_set_slave_address( &ctx->i2c, ctx->slave_address );
    i2c_master_set_speed( &ctx->i2c, cfg->i2c_speed );

    // Input pins

    digital_in_init( &ctx->rdy, cfg->rdy );
    
    return LOADCELL2_OK;
}

void loadcell2_default_cfg ( loadcell2_t *ctx )
{
    // Click default configuration
    
    loadcell2_set_ldo_voltage ( ctx );

    loadcell2_set_gain( ctx, LOADCELL2_GAIN_VAL_x128 );

    loadcell2_set_sample_rate( ctx, LOADCELL2_CONV_RATE_SPS_80 );

    loadcell2_turn_off_clk_chp( ctx );

    loadcell2_enable_dec_cap( ctx );
}

void loadcell2_generic_write ( loadcell2_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len )
{
    uint8_t tx_buf[ 256 ];
    uint8_t cnt;
    
    tx_buf[ 0 ] = reg;
    
    for ( cnt = 1; cnt <= len; cnt++ )
    {
        tx_buf[ cnt ] = data_buf[ cnt - 1 ]; 
    }
    
    i2c_master_write( &ctx->i2c, tx_buf, len + 1 );   
}

void loadcell2_generic_read ( loadcell2_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len )
{
    i2c_master_write_then_read( &ctx->i2c, &reg, 1, data_buf, len );
}

uint8_t loadcell2_check_data_ready ( loadcell2_t *ctx )
{
    uint8_t drdy_stat;

    loadcell2_generic_read( ctx, LOADCELL2_REG_PU_CTRL, &drdy_stat, 1 );
    
    if ( ( drdy_stat && LOADCELL2_CR ) == LOADCELL2_SUCCESS )
    {
        return LOADCELL2_SUCCESS;
    }
    else
    {
        return LOADCELL2_ERROR;
    }
}

void loadcell2_reset ( loadcell2_t *ctx )
{
    uint8_t tmp;
    
    loadcell2_generic_read( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    tmp |= LOADCELL2_RR;
    loadcell2_generic_write( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    dev_rst_delay( );
    
    tmp &= 0xFE;
    loadcell2_generic_write( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
}

LOADCELL2_RETVAL loadcell2_power_on ( loadcell2_t *ctx )
{
    uint8_t tmp;

    loadcell2_generic_read( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    tmp |= LOADCELL2_PUD;
    tmp |= LOADCELL2_PUA;
    loadcell2_generic_write ( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    dev_rst_delay( );
    
    loadcell2_generic_read( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    if ( ( tmp & LOADCELL2_PUR ) != LOADCELL2_ERROR )
    {
        return LOADCELL2_SUCCESS;
    }
    else
    {
        return LOADCELL2_ERROR;
    }
}

void loadcell2_set_ldo_voltage ( loadcell2_t *ctx )
{
    uint8_t tmp;

    loadcell2_generic_read( ctx, LOADCELL2_REG_CTRL1, &tmp, 1 );
    
    tmp &= 0xC7;
    tmp |= 0x20;
    loadcell2_generic_write ( ctx, LOADCELL2_REG_CTRL1, &tmp, 1 );
    
    loadcell2_generic_read( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
    
    tmp |= LOADCELL2_AVDDS;
    loadcell2_generic_write ( ctx, LOADCELL2_REG_PU_CTRL, &tmp, 1 );
}

void loadcell2_set_gain ( loadcell2_t *ctx, uint8_t gain_val )
{
    uint8_t tmp;

    loadcell2_generic_read( ctx, LOADCELL2_REG_CTRL1, &tmp, 1 );
    
    tmp &= 0xF8;
    
    switch ( gain_val )
    {
        case LOADCELL2_GAIN_VAL_x1:
        {
            tmp |= LOADCELL2_GAIN_x1;
            break;
        }
        case LOADCELL2_GAIN_VAL_x2:
        {
            tmp |= LOADCELL2_GAIN_x2;
            break;
        }
        case LOADCELL2_GAIN_VAL_x4:
        {
            tmp |= LOADCELL2_GAIN_x4;
            break;
        }
        case LOADCELL2_GAIN_VAL_x8:
        {
            tmp |= LOADCELL2_GAIN_x8;
            break;
        }
        case LOADCELL2_GAIN_VAL_x16:
        {
            tmp |= LOADCELL2_GAIN_x16;
            break;
        }
        case LOADCELL2_GAIN_VAL_x32:
        {
            tmp |= LOADCELL2_GAIN_x32;
            break;
        }
        case LOADCELL2_GAIN_VAL_x64:
        {
            tmp |= LOADCELL2_GAIN_x64;
            break;
        }
        case LOADCELL2_GAIN_VAL_x128:
        {
            tmp |= LOADCELL2_GAIN_x128;
            break;
        }
        default:
        {
            tmp |= tmp |= LOADCELL2_GAIN_x1;;
            break;
        }
    }
    
    loadcell2_generic_write ( ctx, LOADCELL2_REG_CTRL1, &tmp, 1 );
}

void loadcell2_set_sample_rate ( loadcell2_t *ctx, uint8_t rate_sps )
{
    uint8_t tmp;

    loadcell2_generic_read( ctx, LOADCELL2_REG_CTRL2, &tmp, 1 );

    tmp &= 0x8F;

    switch ( rate_sps )
    {
        case LOADCELL2_CONV_RATE_SPS_10:
        {
            tmp |= LOADCELL2_BIT_CONV_RATE_SPS_10;
            break;
        }
        case LOADCELL2_CONV_RATE_SPS_20:
        {
            tmp |= LOADCELL2_BIT_CONV_RATE_SPS_20;
            break;
        }
        case LOADCELL2_CONV_RATE_SPS_40:
        {
            tmp |= LOADCELL2_BIT_CONV_RATE_SPS_40;
            break;
        }
        case LOADCELL2_CONV_RATE_SPS_80:
        {
            tmp |= LOADCELL2_BIT_CONV_RATE_SPS_80;
            break;
        }
        case LOADCELL2_CONV_RATE_SPS_320:
        {
            tmp |= LOADCELL2_BIT_CONV_RATE_SPS_320;
            break;
        }
        default:
        {
            tmp |= tmp |= LOADCELL2_BIT_CONV_RATE_SPS_10;;
            break;
        }
    }
	
	loadcell2_generic_write ( ctx, LOADCELL2_REG_CTRL2, &tmp, 1 );
}

void loadcell2_turn_off_clk_chp ( loadcell2_t *ctx )
{
    uint8_t tmp;
    
    tmp = 0x30;
    loadcell2_generic_write( ctx, LOADCELL2_REG_ADC_REG, &tmp, 1 );
}

void loadcell2_enable_dec_cap ( loadcell2_t *ctx )
{
    uint8_t tmp;

    loadcell2_generic_read( ctx, LOADCELL2_REG_POW_CTRL, &tmp, 1 );

    tmp &= 0x7F;
    tmp |= 0x80;
    loadcell2_generic_write ( ctx, LOADCELL2_REG_POW_CTRL, &tmp, 1 );
}

void loadcell2_calibrate_afe ( loadcell2_t *ctx )
{
    uint8_t tmp;
    uint8_t check_status;
    uint8_t status_tmp;
    
    check_status = 0;

    loadcell2_generic_read( ctx, LOADCELL2_REG_CTRL2, &tmp, 1 );

    tmp &= 0xFB;
    tmp |= 0x04;
    loadcell2_generic_write ( ctx, LOADCELL2_REG_CTRL2, &tmp, 1 );
    
    while ( check_status )
    {
        loadcell2_generic_read( ctx, LOADCELL2_REG_CTRL2, &status_tmp, 1 );
        dev_rst_delay( );

        status_tmp &= 0xFB;
        status_tmp >>= 2;

        if ( status_tmp == 0 )
        {
             check_status = 1;
        }
    }
}

uint32_t loadcell2_get_result ( loadcell2_t *ctx )
{
    uint8_t rx_buf[ 3 ];
    uint32_t result;
    
    loadcell2_generic_read( ctx, LOADCELL2_REG_ADC_B2, rx_buf, 3 );
    
    result = rx_buf[ 0 ];
    result <<= 8;
    result |= rx_buf[ 1 ];
    result <<= 8;
    result |= rx_buf[ 2 ];
    
    return result;
}

void loadcell2_tare ( loadcell2_t *ctx, loadcell2_data_t *cell_data )
{
    uint32_t results;
    uint8_t n_cnt;
    uint32_t sum_val;
    float average_val;

    for ( n_cnt = 0; n_cnt < 5; n_cnt++ )
    {
        results = loadcell2_get_result( ctx );
        dev_measure_delay( );
    }

    sum_val = 0;

    for ( n_cnt = 0; n_cnt < 100; n_cnt++ )
    {
        results = loadcell2_get_result( ctx );

        sum_val += results;

        dev_measure_delay( );
    }

    average_val = ( float ) sum_val;
    average_val /= 100.0;

    cell_data->tare = average_val;
    cell_data->tare_ok = LOADCELL2_DATA_OK;
    cell_data->weight_data_100g_ok = LOADCELL2_DATA_NO_DATA;
    cell_data->weight_data_500g_ok = LOADCELL2_DATA_NO_DATA;
    cell_data->weight_data_1000g_ok = LOADCELL2_DATA_NO_DATA;
    cell_data->weight_data_5000g_ok = LOADCELL2_DATA_NO_DATA;
    cell_data->weight_data_10000g_ok = LOADCELL2_DATA_NO_DATA;
}

uint8_t loadcell2_calibration ( loadcell2_t *ctx, uint16_t cal_val, loadcell2_data_t *cell_data )
{
    uint32_t results;
    uint8_t n_cnt;
    uint32_t sum_val;
    float average_val;
    float tare_val;
    float weight_val;
    uint8_t status;

    status = LOADCELL2_GET_RESULT_OK;

    tare_val = cell_data->tare;

    sum_val = 0;

    for ( n_cnt = 0; n_cnt < 20; n_cnt++ )
    {
        results = loadcell2_get_result( ctx );

        sum_val += results;

        dev_measure_delay( );
    }

    average_val = ( float ) sum_val;
    average_val /= 20.0;

    weight_val = average_val - tare_val;

    switch ( cal_val )
    {
        case LOADCELL2_WEIGHT_100G :
        {
            cell_data->weight_coeff_100g = 100.0 / weight_val;
            cell_data->weight_data_100g_ok = LOADCELL2_DATA_OK;
            break;
        }
        case LOADCELL2_WEIGHT_500G :
        {
            cell_data->weight_coeff_500g = 500.0 / weight_val;
            cell_data->weight_data_500g_ok = LOADCELL2_DATA_OK;
            break;
        }
        case LOADCELL2_WEIGHT_1000G :
        {
            cell_data->weight_coeff_1000g = 1000.0 / weight_val;
            cell_data->weight_data_1000g_ok = LOADCELL2_DATA_OK;
            break;
        }
        case LOADCELL2_WEIGHT_5000G :
        {
            cell_data->weight_coeff_5000g = 5000.0 / weight_val;
            cell_data->weight_data_5000g_ok = LOADCELL2_DATA_OK;
            break;
        }
        case LOADCELL2_WEIGHT_10000G :
        {
            cell_data->weight_coeff_10000g = 10000.0 / weight_val;
            cell_data->weight_data_10000g_ok = LOADCELL2_DATA_OK;
            break;
        }
        default :
        {
            status = LOADCELL2_GET_RESULT_ERROR;
            cell_data->weight_data_100g_ok = LOADCELL2_DATA_NO_DATA;
            cell_data->weight_data_500g_ok = LOADCELL2_DATA_NO_DATA;
            cell_data->weight_data_1000g_ok = LOADCELL2_DATA_NO_DATA;
            cell_data->weight_data_5000g_ok = LOADCELL2_DATA_NO_DATA;
            cell_data->weight_data_10000g_ok = LOADCELL2_DATA_NO_DATA;
            break;
        }
    }

    return status;
}

float loadcell2_get_weight ( loadcell2_t *ctx, loadcell2_data_t *cell_data )
{
    uint32_t results;
    uint8_t n_cnt;
    uint32_t sum_val;
    float average_val;
    float tare_val;
    float weight_val;
    uint8_t status;

    status = LOADCELL2_GET_RESULT_OK;

    tare_val = cell_data->tare;

    sum_val = 0;

    for ( n_cnt = 0; n_cnt < 20; n_cnt++ )
    {
        results = loadcell2_get_result( ctx );

        sum_val += results;

        dev_measure_delay( );
    }

    average_val = ( float ) sum_val;
    average_val /= 20.0;

    weight_val = average_val - tare_val;

    if ( cell_data->weight_data_100g_ok == LOADCELL2_DATA_OK )
    {
        weight_val *= cell_data->weight_coeff_100g;
    }
    else if ( cell_data->weight_data_500g_ok == LOADCELL2_DATA_OK )
    {
        weight_val *= cell_data->weight_coeff_500g;
    }
    else if ( cell_data->weight_data_1000g_ok == LOADCELL2_DATA_OK )
    {
        weight_val *= cell_data->weight_coeff_1000g;
    }
    else if ( cell_data->weight_data_5000g_ok == LOADCELL2_DATA_OK )
    {
        weight_val *= cell_data->weight_coeff_5000g;
    }
    else if ( cell_data->weight_data_10000g_ok == LOADCELL2_DATA_OK )
    {
        weight_val *= cell_data->weight_coeff_10000g;
    }
    else
    {
        weight_val *= LOADCELL2_DEFAULT_WEIGHT_SCALE_COEFFICIENT;
    }

    if ( weight_val < 0 )
    {
        weight_val = 0.0;
    }

    return weight_val;
}

uint8_t loadcell2_check_drdy ( loadcell2_t *ctx )
{
    return  digital_in_read( &ctx->rdy ); 
}

// ----------------------------------------------- PRIVATE FUNCTION DEFINITIONS

static void dev_rst_delay( void )
{
    Delay_1ms( );
}

static void dev_measure_delay ( void )
{
    Delay_1ms( );
}

// ------------------------------------------------------------------------- END

