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
 * @file  hal_target.h
 * @brief HAL target macros and typedefs.
 */

#ifndef _HAL_TARGET_H_
#define _HAL_TARGET_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "hal_ll_target.h"

#define HAL_MODULE_ERROR (hal_base_addr_t)(0xFFFFFFFF) /*!< @def General module error. */
#define HAL_CHANNEL_ERROR (hal_channel_t)(0xFFFFFFFF) /*!< @def Channel error. ( Timer, ADC... ) */
#define HAL_PIN_NC (hal_pin_name_t)(0xFFFFFFFF) /*!< @def Pin error. ( Wrong pin selected ) */
#define HAL_PORT_NC (hal_port_name_t)(0xFFFFFFFF) /*!< @def Port error. ( Wrong port selected ) */

typedef hal_ll_base_addr_t hal_base_addr_t; /*!< Base address, which is size dependant on the architecture. */
typedef hal_ll_channel_t hal_channel_t; /*!< Channel type, which is size dependant on the architecture. */
typedef hal_ll_pin_name_t hal_pin_name_t; /*!< Pin type, which is size dependant on the architecture. */
typedef hal_ll_port_name_t hal_port_name_t; /*!< Port type, which is size dependant on the architecture. */
typedef hal_ll_port_size_t hal_port_size_t; /*!< Port width, which is size dependant on the architecture. */

typedef int32_t err_t; /*!< General error type. */

/**
 * @details Predefined enum values for acquiring adequate object handle.
 */
typedef enum
{
    ACQUIRE_SUCCESS = 0,  /*!< Acquire successful. */
    ACQUIRE_INIT,         /*!< Init required. */

    ACQUIRE_FAIL = (-1)   /*!< Acquire failed. */
} acquire_t;

#ifdef __cplusplus
}
#endif

#endif // _HAL_TARGET_H_
// ------------------------------------------------------------------------- END
