/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mt3620-baremetal.h"
#include "mt3620-adc.h"
#include "mt3620-timer-poll.h"

static const uintptr_t ADC_CTRL_BASE = 0x38000100;

typedef enum {
    ADC_CTL0 = 0x0,
    ADC_CTL1 = 0x4,
    ADC_CTL2 = 0x8,
    ADC_CTL3 = 0xC,
    ADC_FIFO_RBR = 0x100,
    ADC_FIFO_TRI_LVL = 0x160,
    ADC_FIFO_DEBUG16 = 0x1D4
} AdcReg;

static inline uint32_t Bit(size_t index)
{
    return UINT32_C(1) << index;
}

static inline uint32_t BitMask(size_t high, size_t low)
{
    uint32_t highToZero = Bit(high + 1) - 1;
    uint32_t lowMinusOneToZero = Bit(low) - 1;
    uint32_t mask = highToZero &= ~lowMinusOneToZero;

    return mask;
}

static inline uint32_t ReadAdcReg32(AdcReg reg)
{
    return ReadReg32(ADC_CTRL_BASE, reg);
}

static inline void WriteAdcReg32(AdcReg reg, uint32_t value)
{
    WriteReg32(ADC_CTRL_BASE, reg, value);
}

void EnableAdc(void)
{
    // Select clocks and other input parameters.
    uint32_t adc_ctl3 = ReadAdcReg32(ADC_CTL3);

    adc_ctl3 &= ~Bit(31);       // VREF18 supplied by 2.5V (AVDD)
    adc_ctl3 &= ~Bit(17);       // select ADC controller clock source
    adc_ctl3 &= ~Bit(16);       // keep original clock
    adc_ctl3 |= Bit(15);        // generate difference clock phase for ADC
    adc_ctl3 |= Bit(13);        // enable VCM (common-mode voltage) generator
    adc_ctl3 |= Bit(11);        // AUXADC input MUX enable
    adc_ctl3 &= ~BitMask(9, 8); // dithering function step size 8
    adc_ctl3 |= UINT32_C(2) << 8;
    adc_ctl3 |= Bit(6);         // enable dithering function
    adc_ctl3 |= Bit(4);         // enable comparator pre-amplifier
    adc_ctl3 &= ~BitMask(3, 2); // comparator pre-amplifier current 80uA
    adc_ctl3 |= UINT32_C(1) << 2;
    adc_ctl3 &= ~BitMask(1, 0); // comparator timing loop delay time 6ns
    adc_ctl3 |= UINT32_C(1) << 0;

    WriteAdcReg32(ADC_CTL3, adc_ctl3);

    // Disable all input channels, set averaging, and set stabilization.
    uint32_t adc_ctl0 = ReadAdcReg32(ADC_CTL0);

    adc_ctl0 &= ~BitMask(31, 16); // [REG_CH_MAP] = 0x0000 -> disable all channels
    adc_ctl0 &= ~Bit(8);          // [PMODE] = 0 -> disable periodic timer
    adc_ctl0 &= ~BitMask(3, 1);   // [REG_AVG_MODE] = 0 -> REG_AVG_NUMBER = 1
    adc_ctl0 &= ~BitMask(15, 9);  // [REG_T_INIT] = 20 (default)
    adc_ctl0 |= UINT32_C(20) << 9;

    WriteAdcReg32(ADC_CTL0, adc_ctl0);

    // From datasheet, "wait 100 clock cycles for ADC reference generator settled".
    // 100 cycles @ 2MHz = 50us.
    Gpt3_WaitUs(50);
}

static size_t FifoEntryCount(void)
{
    uint32_t debug16 = ReadAdcReg32(ADC_FIFO_DEBUG16);
    uint32_t readPtr = debug16 & 0x1F;
    uint32_t writePtr = (debug16 >> 5) & 0x1F;

    size_t entryCount;
    if (writePtr >= readPtr) {
        entryCount = writePtr - readPtr;
    } else {
        // The five bits available to the read and write pointers allow
        // a greater range than the actual FIFO size.
        static const size_t ADC_FIFO_ADDRESS_SIZE = 1U << 5;
        entryCount = writePtr + ADC_FIFO_ADDRESS_SIZE - readPtr;
    }

    return entryCount;
}

uint32_t ReadAdc(uint8_t channel)
{
    // Drain any existing data from the RX FIFO.
    for (size_t i = FifoEntryCount(); i > 0; --i) {
        ReadAdcReg32(ADC_FIFO_RBR);
    }

    // Select channel and enable the FSM.
    uint32_t adc_ctl0 = ReadAdcReg32(ADC_CTL0);
    adc_ctl0 &= ~BitMask(31, 16); // [REG_CH_MAP] = channelMask
    adc_ctl0 |= Bit(16 + channel);
    adc_ctl0 |= Bit(0); // [ADC_FSM_EN] = 1 -> start FSM
    WriteAdcReg32(ADC_CTL0, adc_ctl0);

    // From datasheet,
    // "4. wait 8 clock cycles for channel switches settled & ADC latency (2 clock cycles)
    //  5.. 32 clock cycles for averaging"
    // 8 + 2 + 32 = 42 cycles, @2MHz = 21us
    Gpt3_WaitUs(21);

    while (FifoEntryCount() == 0) {
        // empty.
    }

    // Disable FSM.
    adc_ctl0 = ReadAdcReg32(ADC_CTL0);
    adc_ctl0 &= ~Bit(0); // [ADC_FSM_EN] = 0 -> disable FSM
    WriteAdcReg32(ADC_CTL0, adc_ctl0);

    uint32_t rbr = ReadAdcReg32(ADC_FIFO_RBR);
    uint32_t rbrChannel = rbr & 0xF;        // ADC_FIFO_RBR[3:0] = channel number
    uint32_t rbrValue = (rbr >> 4) & 0xFFF; // ADC_FIFO_RBR[15:4] = sample

    // If unexpected channel number then return UINT32_MAX to indicate error.
    if (rbrChannel != channel) {
        return UINT32_MAX;
    }

    return rbrValue;
}
