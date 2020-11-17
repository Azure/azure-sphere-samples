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

#include "relay.h"

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void relay_cfg_setup ( relay_cfg_t *cfg )
{
    // Additional gpio pins

    cfg->rel2   = HAL_PIN_NC;
    cfg->rel1   = HAL_PIN_NC;
}

RELAY_RETVAL relay_init ( relay_t *ctx, relay_cfg_t *cfg )
{
    // Output pins 

    digital_out_init( &ctx->rel2, cfg->rel2 );
    digital_out_init( &ctx-> rel1, cfg->rel1 );

    return RELAY_OK;
}

void relay_default_cfg ( relay_t *ctx )
{
    // Click default configuration

    digital_out_low( &ctx->rel1 );
    digital_out_low( &ctx->rel2 );
}

void relay_set_state ( relay_t *ctx, uint8_t relay, uint8_t state )
{
    switch (relay)
    {
        case RELAY_NUM_1:
        {
            digital_out_write( &ctx->rel1, state );
            break;
        }
        case RELAY_NUM_2:
        {
            digital_out_write( &ctx->rel2, state );
            break;
        }
    }
}

// ------------------------------------------------------------------------- END

