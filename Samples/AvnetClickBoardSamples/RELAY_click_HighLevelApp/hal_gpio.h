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
 * @file  hal_gpio.h
 * @brief This file contains all the functions prototypes for the GPIO library.
 */

#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "hal_target.h"
#include "hal_ll_gpio.h"

/**
 * @details Predefined enum values for for pin direction selection.
 */
typedef enum
{
    HAL_GPIO_DIGITAL_INPUT = 0, /*!< GPIO as digital input. */
    HAL_GPIO_DIGITAL_OUTPUT = 1 /*!< GPIO as digital output. */
} hal_gpio_direction_t;

typedef handle_t hal_gpio_base_t; /*!< Handle type. */
typedef hal_ll_gpio_mask_t hal_gpio_mask_t; /*!< Mask type. */

/**
 * @brief GPIO HAL context structure, consisted of the following fields :
 * @details User needs to specify values herein.
 * @note The values are specified by #hal_gpio_configure_pin and
 * #hal_gpio_configure_port.
 * @warning  The contents of the context structure are used by the module and
 * must not be altered. Reading or writing data directly from a control structure
 * by user should be avoided.
 */
typedef struct hal_gpio_t
{
    hal_gpio_base_t base; /*!< Port base address. */
    hal_gpio_mask_t mask; /*!< Port bit mask. */
};

typedef struct hal_gpio_t hal_gpio_pin_t;  /*!< Forward declaration of the gpio pin typedef. */
typedef struct hal_gpio_t hal_gpio_port_t; /*!< Forward declaration of the gpio port typedef. */

/*!
 * @addtogroup pergroup Microcontroller Peripherals
 * @{
 */

/*!
 * @addtogroup halgroup Hardware Abstraction Layer
 * @{
 */

/*!
 * @addtogroup halgpiogroup GPIO HAL
 * @brief GPIO Hardware Abstraction Layer API Reference.
 * @details API for configuring and manipulating GPIO HAL module.
 * @{
 */

/**
 * @brief Configure pin.
 * @details Configure pin as digital input or output.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @param[in] name Pin name.
 * @param[in] direction GPIO pin direction.
 * See #hal_gpio_direction_t for valid values.
 * @return Nothing.
 *
 * @b Predefined @b values @b for @b direction:
 * Function            | Default value             |
 * --------------------|---------------------------|
 * input               | HAL_GPIO_DIGITAL_INPUT    |
 * output              | HAL_GPIO_DIGITAL_OUTPUT   |
 *
 * @pre Make sure that \p pin structure has been declared.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @warning The following example includes pin mapping.
 * Take into consideration that different hardware might not have the same pins.
 * Make sure to accommodate pin name based on your hardware specifics.
 *
 * @b Example
 * @code
 *   static hal_gpio_pin_t *pin;
 *
 *   // Configures pin as digital output.
 *   hal_gpio_configure_pin( &pin, PB2, HAL_GPIO_DIGITAL_OUTPUT );
 * @endcode
 */
void hal_gpio_configure_pin( hal_gpio_pin_t *pin, hal_pin_name_t name, hal_gpio_direction_t direction );

/**
 * @brief Read pin.
 * @details Reads the current pin input level.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @return Function returns pin input logic level.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   uint8_t value;
 *   static hal_gpio_pin_t *pin;
 *
 *   // Reads pin state.
 *   value = hal_gpio_read_pin_input( &pin );
 * @endcode
 */
uint8_t hal_gpio_read_pin_input( hal_gpio_pin_t *pin );

/**
 * @brief Read pin.
 * @details Reads the current pin output level.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @return Function returns pin output logic level.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   uint8_t value;
 *   static hal_gpio_pin_t *pin;
 *
 *   // Reads pin state.
 *   value = hal_gpio_read_pin_output( &pin );
 * @endcode
 */
uint8_t hal_gpio_read_pin_output( hal_gpio_pin_t *pin );

/**
 * @brief Sets pin state.
 * @details Sets the current output logic of the GPIO pin to 0 or 1.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @param[in] value Pin state, 0 or 1.
 * @return Nothing.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   static hal_gpio_pin_t *pin;
 *
 *   // Set pin logic state to high (1).
 *   hal_gpio_write_pin_output( &pin, 1 );
 * @endcode
 */
void hal_gpio_write_pin_output( hal_gpio_pin_t *pin, uint8_t value );

/**
 * @brief Toggle pin state.
 * @details Toggles the current output logic of the GPIO pin.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @return Nothing.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   static hal_gpio_pin_t *pin;
 *
 *   // Toggle pin logic state.
 *   hal_gpio_toggle_pin_output( &pin );
 * @endcode
 */
void hal_gpio_toggle_pin_output( hal_gpio_pin_t *pin );

/**
 * @brief Set pin state high.
 * @details Sets the current output logic of the GPIO pin to 1.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @return Nothing.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   static hal_gpio_pin_t *pin;
 *
 *   // Set pin logic state high (1).
 *   hal_gpio_set_pin_output( &pin );
 * @endcode
 */
void hal_gpio_set_pin_output( hal_gpio_pin_t *pin );

/**
 * @brief Set pin state low.
 * @details Sets the current output logic of the GPIO pin to 0.
 * @param[in,out] pin GPIO HAL pin context structure.
 * See #hal_gpio_pin_t structure definition for detailed explanation.
 * @return Nothing.
 * @pre Make sure that \p pin structure has been configured.
 * See #hal_gpio_configure_pin for detailed explanation.
 *
 * @b Example
 * @code
 *   static hal_gpio_pin_t *pin;
 *
 *   // Set pin logic state low (0).
 *   hal_gpio_clear_pin_output( &pin );
 * @endcode
 */
void hal_gpio_clear_pin_output( hal_gpio_pin_t *pin );

/**
 * @brief Configure port.
 * @details Configure port as digital input or output.
 * @param[in,out] port GPIO HAL port context structure.
 * See #hal_gpio_port_t structure definition for detailed explanation.
 * @param[in] name Port name.
 * @param[in] mask Port bit mask.
 * See #hal_gpio_mask_t structure definition for detailed explanation.
 * @param[in] direction GPIO pin direction.
 * See #hal_gpio_direction_t structure definition for detailed explanation.
 * @return Nothing.
 *
 * @b Predefined @b values @b for @b direction:
 * Function            | Default value             |
 * --------------------|---------------------------|
 * input               | HAL_GPIO_DIGITAL_INPUT    |
 * output              | HAL_GPIO_DIGITAL_OUTPUT   |
 *
 * @pre Make sure that \p port structure has been declared.
 * See #hal_gpio_port_t structure definition for detailed explanation.
 * @warning The following example includes pin mapping.
 * Take into consideration that different hardware might not have the same pins.
 * Make sure to accommodate pin name based on your hardware specifics.
 *
 * @b Example
 * @code
 *   hal_port_size_t value;
 *   static hal_gpio_port_t *port;
 *
 *   // Configures PORTB pins 0..7 as digital output.
 *   hal_gpio_configure_port( &port, PORTB, 0xFF,HAL_GPIO_DIGITAL_OUTPUT );
 * @endcode
 */
void hal_gpio_configure_port( hal_gpio_port_t *port, hal_port_name_t name, hal_gpio_mask_t mask, hal_gpio_direction_t direction );

/**
 * @brief Read port.
 * @details Reads the current input logic of the GPIO port.
 * @param[in,out] port GPIO HAL port context structure.
 * See #hal_gpio_port_t structure definition for detailed explanation.
 * @return Function returns port output state depending on the MCU architecture.
 * @pre Make sure that \p port structure has been configured.
 * See #hal_gpio_configure_port for detailed explanation.
 *
 * @b Example
 * @code
 *   hal_port_size_t value;
 *   static hal_gpio_port_t *port;
 *
 *   // Reads port state.
 *   value = hal_gpio_read_port_input( &port );
 * @endcode
 */
hal_port_size_t hal_gpio_read_port_input( hal_gpio_port_t *port );

/**
 * @brief Read port.
 * @details Reads the current output logic of the GPIO port.
 * @param[in,out] port GPIO HAL port context structure.
 * See #hal_gpio_port_t structure definition for detailed explanation.
 * @return Function returns port output state depending on the MCU architecture.
 * @pre Make sure that \p port structure has been configured.
 * See #hal_gpio_configure_port for detailed explanation.
 *
 * @b Example
 * @code
 *   hal_port_size_t value;
 *   static hal_gpio_port_t *port;
 *
 *   // Reads port state.
 *   value = hal_gpio_read_port_output( &port );
 * @endcode
 */
hal_port_size_t hal_gpio_read_port_output( hal_gpio_port_t *port );

/**
 * @brief Sets port state.
 * @details Sets the current output logic of the GPIO port to 0.
 * @param[in,out] port GPIO HAL port context structure.
 * See #hal_gpio_port_t structure definition for detailed explanation.
 * @param[in] value Port state / mask.
 * See #hal_port_size_t structure definition for detailed explanation.
 * @return Nothing.
 * @pre Make sure that \p port structure has been configured.
 * See #hal_gpio_configure_port for detailed explanation.
 *
 * @b Example
 * @code
 *   static hal_gpio_port_t *port;
 *
 *   // Set port logic state to 0xAA.
 *   hal_gpio_write_port_output( &port, 0xAA );
 * @endcode
 */
void hal_gpio_write_port_output( hal_gpio_port_t *port, hal_port_size_t value );

/*! @} */ // halgpiogroup
/*! @} */ // halgroup
/*! @} */ // pergroup

#ifdef __cplusplus
}
#endif

#endif // _HAL_GPIO_H_
// ------------------------------------------------------------------------- END
