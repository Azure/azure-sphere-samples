/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "mt3620-gpio.h"

// The location of the DIN register depends on the type of block.
typedef enum {
    GpioRegAdcDin = 0x04, // PAD GPI Input Data Control Register
    GpioRegPwmDin = 0x04, // GPIO PAD Input Value Register
    GpioRegGrpDin = 0x04, // GPIO PAD Input Value Register
    GpioRegIsuDin = 0x0C, // PAD GPI Input Data Control Register
    GpioRegI2SDin = 0x00, // PAD GPI Input Data Control Register

    GpioRegDoutSet = 0x14,   // PAD GPO DATA Output Control Set Register
    GpioRegDoutReset = 0x18, // PAD GPO DATA Output Control Reset Register
    GpioRegOe = 0x20,        // PAD GPO Output Enable Control Register
    GpioRegOeSet = 0x24,     // PAD GPO Output Enable Set Control Register
    GpioRegOeReset = 0x28,   // PAD GPO Output Enable Reset Control Register
    GpioRegIes = 0x60,       // PAD IES Control Register
    GpioRegIesSet = 0x64,    // PAD IES SET Control Register
    GpioRegIesReset = 0x68,  // PAD IES RESET Control Register
} GpioReg;

typedef struct {
    GpioReg dinReg;
} BlockType;

// Indices correspond to values in mt3620-gpio.h.
static const BlockType blockTypes[] = {[GpioBlock_ADC] = {.dinReg = GpioRegAdcDin},
                                       [GpioBlock_PWM] = {.dinReg = GpioRegPwmDin},
                                       [GpioBlock_GRP] = {.dinReg = GpioRegGrpDin},
                                       [GpioBlock_ISU] = {.dinReg = GpioRegIsuDin},
                                       [GpioBlock_I2S] = {.dinReg = GpioRegI2SDin}};

typedef struct {
    const GpioBlock *block; // NULL if not used.
} PinInfo;

#define GPIO_COUNT 76
static PinInfo pins[GPIO_COUNT];

// ---- register access ----

// Multiple GPIO pins are controlled by a single register. This function
// returns the associated block, and the index mask of the supplied pin within
// that block.
static const GpioBlock *PinIdToBlock(int gpioId, PinInfo **pinInfo, uint32_t *mask)
{
    if (gpioId < 0 || gpioId >= GPIO_COUNT) {
        return NULL;
    }

    PinInfo *pi1 = &pins[gpioId];
    const GpioBlock *block = pi1->block;
    if (block == NULL) {
        return NULL;
    }

    if (pinInfo) {
        *pinInfo = pi1;
    }

    if (mask) {
        int idx = gpioId - block->firstPin;
        *mask = UINT32_C(1) << idx;
    }

    return block;
}

static volatile uint32_t *BlockRegToPtr32(const GpioBlock *block, GpioReg offset)
{
    uintptr_t addr = block->baseAddr + offset;
    return (volatile uint32_t *)addr;
}

static void Gpio_WriteReg32(const GpioBlock *block, GpioReg reg, uint32_t value)
{
    volatile uint32_t *p = BlockRegToPtr32(block, reg);
    *p = value;
}

static uint32_t Gpio_ReadReg32(const GpioBlock *block, GpioReg reg)
{
    const volatile uint32_t *p = BlockRegToPtr32(block, reg);
    uint32_t value = *p;
    return value;
}

// ---- pin configuration / status ----

static int ConfigurePin(int pin, bool asInput)
{
    PinInfo *pinInfo;
    uint32_t pinMask;
    const GpioBlock *block = PinIdToBlock(pin, &pinInfo, &pinMask);
    if (block == NULL) {
        return -ENOENT;
    }

    Gpio_WriteReg32(block, GpioRegOeReset, pinMask);
    Gpio_WriteReg32(block, GpioRegIesReset, pinMask);

    if (asInput) {
        Gpio_WriteReg32(block, GpioRegIesSet, pinMask);
    } else {
        Gpio_WriteReg32(block, GpioRegOeSet, pinMask);
    }

    return 0;
}

int Mt3620_Gpio_ConfigurePinForOutput(int pin)
{
    return ConfigurePin(pin, /* asInput */ false);
}

int Mt3620_Gpio_ConfigurePinForInput(int pin)
{
    return ConfigurePin(pin, /* asInput */ true);
}

int Mt3620_Gpio_Write(int pin, bool state)
{
    uint32_t pinMask;
    const GpioBlock *block = PinIdToBlock(pin, NULL, &pinMask);
    if (block == NULL) {
        return -ENOENT;
    }

    GpioReg reg = state ? GpioRegDoutSet : GpioRegDoutReset;
    Gpio_WriteReg32(block, reg, pinMask);

    return 0;
}

int Mt3620_Gpio_Read(int pin, bool *state)
{
    PinInfo *pinInfo;
    uint32_t pinMask;
    const GpioBlock *block = PinIdToBlock(pin, &pinInfo, &pinMask);
    if (block == NULL) {
        return -ENOENT;
    }

    GpioReg dinReg = blockTypes[pinInfo->block->type].dinReg;
    uint32_t din = Gpio_ReadReg32(block, dinReg);
    *state = ((din & pinMask) != 0);
    return 0;
}

// ---- initialization ----

int Mt3620_Gpio_AddBlock(const GpioBlock *block)
{
    int low = block->firstPin;
    int high = block->firstPin + block->pinCount - 1;

    if (low < 0 || high >= GPIO_COUNT) {
        return -ENOENT;
    }

    for (int pin = low; pin <= high; ++pin) {
        if (pins[pin].block != NULL) {
            return -EEXIST;
        }

        pins[pin].block = block;
    }

    return 0;
}
