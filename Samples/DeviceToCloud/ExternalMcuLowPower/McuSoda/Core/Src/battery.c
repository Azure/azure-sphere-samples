/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

float ReadBatteryLevel(void)
{
	float batteryLevel = 0.0f;
	HAL_ADC_Start(&hadc);

	// Turn on battery monitoring circuit
	HAL_GPIO_WritePin(ENA_BATTERY_LVL_GPIO_Port, ENA_BATTERY_LVL_Pin, GPIO_PIN_SET);

	// Read ADC
	if (HAL_ADC_PollForConversion(&hadc, 10) != HAL_OK) {
		Error_Handler();
	}

	if ((HAL_ADC_GetState(&hadc) & HAL_ADC_STATE_REG_EOC) == HAL_ADC_STATE_REG_EOC) {
		uint16_t readAdcValue = HAL_ADC_GetValue(&hadc);

		// The ADC reads values in the range 0-4096, corresponding to 0-3.3V.
		// The battery circuit also incorporates a halving voltage divider, so we
		// must double the reading before scaling by 3.3/4096 to get the battery voltage.
		batteryLevel = (readAdcValue * 2 * 3.3) / 4096;
	}

	// Turn off Battery monitoring circuit
	HAL_GPIO_WritePin(ENA_BATTERY_LVL_GPIO_Port, ENA_BATTERY_LVL_Pin, GPIO_PIN_RESET);
	HAL_ADC_Stop(&hadc);

	return batteryLevel;
}
