/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_ADC_H
#define MT3620_ADC_H

#include <stdint.h>

/// <summary>
/// Enable the ADC with a 2.5V reference value and one sample averaging.
/// </summary>
void EnableAdc(void);

/// <summary>
/// <para>Read a single sample from the ADC on the supplied channel.</para>
/// <para>Call <see cref="EnableAdc" /> to configure the reference voltage
/// and averaging mode before calling this function.</para>
/// </summary>
/// <param name="channel">Which ADC channel to read the sample from.</param>
/// <returns>The sample value.  This is a 12-bit value between 0x000 and 0xFFF,
/// which is a proportion of the reference voltage.  With the reference voltage
/// set to 2.5V, a sample value of 0x800 means a 1.25V signal was sampled.
/// If an error occurs, returns 0xFFFFFFFF.</returns>
uint32_t ReadAdc(uint8_t channel);

#endif // #ifndef MT3620_ADC_H
