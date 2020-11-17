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
 * @file  hal_gpio.c
 * @brief This file contains all the functions prototypes for the GPIO library.
 */

#include "hal_gpio.h"

void hal_gpio_configure_pin( hal_gpio_pin_t *pin, hal_pin_name_t name,
                                                  hal_gpio_direction_t direction )
{
    hal_ll_gpio_configure_pin( pin, name, direction );
}

uint8_t hal_gpio_read_pin_input( hal_gpio_pin_t *pin )
{
    if ( pin->base != NULL )
    {
        return hal_ll_gpio_read_pin_input( pin );
    }
}

uint8_t hal_gpio_read_pin_output( hal_gpio_pin_t *pin )
{
    if ( pin->base != NULL )
    {
        return hal_ll_gpio_read_pin_output( pin );
    }
}

void hal_gpio_write_pin_output( hal_gpio_pin_t *pin, uint8_t value )
{
    if ( pin->base != NULL )
    {
        hal_ll_gpio_write_pin_output( pin, value );
    }
}

void hal_gpio_toggle_pin_output( hal_gpio_pin_t *pin )
{
    if ( pin->base != NULL )
    {
        hal_ll_gpio_toggle_pin_output( pin );
    }
}

void hal_gpio_set_pin_output( hal_gpio_pin_t *pin )
{
    if ( pin->base != NULL )
    {
        hal_ll_gpio_set_pin_output( pin );
    }
}

void hal_gpio_clear_pin_output( hal_gpio_pin_t *pin )
{
    if ( pin->base != NULL )
    {
        hal_ll_gpio_clear_pin_output( pin );
    }
}

void hal_gpio_configure_port( hal_gpio_port_t *port, hal_port_name_t name,
                              hal_gpio_mask_t mask, hal_gpio_direction_t direction )
{
    hal_ll_gpio_configure_port( port, name, mask, direction );
}

hal_port_size_t hal_gpio_read_port_input( hal_gpio_port_t *port )
{
    if ( port->base != NULL )
    {
        return hal_ll_gpio_read_port_input( port );
    }
}

hal_port_size_t hal_gpio_read_port_output( hal_gpio_port_t *port )
{
    if ( port->base != NULL )
    {
        return hal_ll_gpio_read_port_output( port );
    }
}

void hal_gpio_write_port_output( hal_gpio_port_t *port, hal_port_size_t value )
{
    if ( port->base != NULL )
    {
        hal_ll_gpio_write_port_output( port, value );
    }
}

// ------------------------------------------------------------------------- END
