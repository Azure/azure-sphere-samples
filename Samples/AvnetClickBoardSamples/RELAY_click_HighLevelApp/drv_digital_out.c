/****************************************************************************
**
** Copyright (C) 2020 MikroElektronika d.o.o.
** Contact: https://www.mikroe.com/contact
**
** This file is part of the mikroSDK package
**
** Commercial License Usage
**
** Licensees holding valid commercial NECTO compilers AI licenses may use this
** file in accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The mikroElektronika Company.
** For licensing terms and conditions see
** https://www.mikroe.com/legal/software-license-agreement.
** For further information use the contact form at
** https://www.mikroe.com/contact.
**
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used for
** non-commercial projects under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** OF MERCHANTABILITY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
** OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
/*!
 * @file  drv_digital_out.c
 * @brief Digital output driver implementation.
 */

#include "drv_digital_out.h"

err_t digital_out_init( digital_out_t *out, pin_name_t name )
{
    if ( name == HAL_PIN_NC )
    {
        return DIGITAL_OUT_UNSUPPORTED_PIN;
    }

    hal_gpio_configure_pin( &out->pin, name, GPIO_DIGITAL_OUTPUT );
    return DIGITAL_OUT_SUCCESS;
}

void digital_out_high( digital_out_t *out )
{
    hal_gpio_set_pin_output( &out->pin );
}

void digital_out_low( digital_out_t *out )
{
    hal_gpio_clear_pin_output( &out->pin );
}

void digital_out_toggle( digital_out_t *out )
{
    hal_gpio_toggle_pin_output( &out->pin );
}

void digital_out_write( digital_out_t *out, uint8_t value )
{
    hal_gpio_write_pin_output( &out->pin, value );
}

// ------------------------------------------------------------------------- END
