/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_GPIO_H
#define MT3620_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/// <summary>
/// The MT3620 board supports multiple GPIO block types. These blocks
/// have slightly different register layouts, so the GPIO software needs
/// to know what kind of block a given pin is in.
/// </summary>
typedef enum {
    /// <summary>GPIO pins are multiplexed with an ADC block.</summary>
    GpioBlock_ADC = 0,
    /// <summary>GPIO block also supports PWM.</summary>
    GpioBlock_PWM = 1,
    /// <summary>A plain GPIO block.</summary>
    GpioBlock_GRP = 2,
    /// <summary>GPIO pins are multiplexed with I2C / SPI / UART.</summary>
    GpioBlock_ISU = 3,
    /// <summary>GPIO pins are multiplexed with I2S block.</summary>
    GpioBlock_I2S = 4
} GpioBlockType;

/// <summary>
/// <para>Each pin belongs to a GPIO block, and each block has multiple contiguous
/// pins. This structure describes a block's physical layout and type.</para>
/// <para>The application must call <see cref="Mt3620_Gpio_AddBlock" /> once for each
/// block that it uses, before configuring or using the pins in that block.</para>
/// </summary>
typedef struct {
    /// <summary>The start of the block's register bank.</summary>
    uintptr_t baseAddr;
    /// <summary>The type of block. This describes how the registers are laid out.</summary>
    GpioBlockType type;
    /// <summary>First pin in this block. Each block contains a contiguous range of pins.</summary>
    uint8_t firstPin;
    /// <summary>Number of pins in this block. The first pin is given by <see cref="firstPin" />
    /// and the last pin is firstPin + pinCount - 1.</summary>
    uint8_t pinCount;
} GpioBlock;

/// <summary>
/// <para>An application must call this function before it configures or uses any of
/// the pins in the block. This function only needs to be called once for each block.</para>
/// <para>**Errors**</para>
/// <para>-ENOENT if the block range contains an unsupported pin.</para>
/// <para>-EEXIST if any of the pins in the block range has already been claimed by
/// a previous call to <see cref="Mt3620_Gpio_AddBlock" />.</para>
/// </summary>
/// <param name="block">Describes a contiguous range of pins.</param>
/// <returns>Zero on success, A negative error code on failure.</returns>
int Mt3620_Gpio_AddBlock(const GpioBlock *block);

/// <summary>
/// <para>Configure a pin for output. Call <see cref="Mt3620_Gpio_Write" /> to set the
/// state.</para>
/// <para><see cref="Mt3620_Gpio_AddBlock" /> must be called before this function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <returns>Zero on success, a standard errno.h code otherwise.</returns>
int Mt3620_Gpio_ConfigurePinForOutput(int pin);

/// <summary>
/// <para>Configure a pin for input. Call <see cref="Mt3620_Gpio_Write" /> to read the
/// state.</para>
/// <para>This function does not control the pull-up or pull-down resistors.
/// If the pin is connected to a possibly-floating input, the application may
/// want to additionally enable these via the register interface.</para>
/// <para><see cref="Mt3620_Gpio_AddBlock" /> must be called before this function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <returns>Zero on success, a standard errno.h code otherwise.</returns>
int Mt3620_Gpio_ConfigurePinForInput(int pin);

/// <summary>
/// <para>Set the state of a pin which has been configured for output.</para>
/// <para><see cref="Mt3620_Gpio_ConfigurePinForOutput" /> must be called before this
/// function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <param name="state">true to drive the pin high; false to drive it low.</param>
/// <returns>Zero on success, a standard errno.h code otherwise.</returns>
int Mt3620_Gpio_Write(int pin, bool state);

/// <summary>
/// <para>Read the state of a pin which has been configured for input.</para>
/// <para><see cref="Mt3620_Gpio_ConfigurePinForInput" /> must be called before this
/// function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <param name="state">On return, contains true means the input is high, and false means
/// low.</param>
/// <returns>Zero on success, a standard errno.h code otherwise.</returns>
int Mt3620_Gpio_Read(int pin, bool *state);

#endif // #ifndef MT3620_GPIO_H
