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
 * @file  drv_name.h
 * @brief Pin and port name type definitions.
 */

#ifndef _DRV_NAME_H_
#define _DRV_NAME_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "hal_target.h"

/*!
 * @addtogroup pergroup Microcontroller Peripherals
 */

/*!
 * @addtogroup drvgroup Driver Layer
 */

/*!
 * @addtogroup halgroup Hardware Abstraction Layer
 */

/**
 * @details Pin direction enum.
 */
typedef enum
{
    GPIO_DIGITAL_INPUT = 0, /*!< GPIO Digital input. */
    GPIO_DIGITAL_OUTPUT = 1 /*!< GPIO Digital output. */
} gpio_direction_t;

typedef hal_pin_name_t pin_name_t; /*!< GPIO pin name. */

typedef hal_port_name_t port_name_t; /*!< GPIO port name.*/

typedef hal_port_size_t port_size_t; /*!< GPIO port size. */

#ifdef __cplusplus
}
#endif

#endif // _DRV_NAME_H_
// ------------------------------------------------------------------------- END
