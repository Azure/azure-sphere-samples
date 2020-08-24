/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

static void SetFlagIfDebounceExpired(uint32_t *lastIsrTime, __IO bool *event);
static void WakeUpMT3620(void);

// The last time (in ticks) when an interrupt (button, wakeup, or UART) occurred.
__IO uint32_t lastActivity = NO_PREV_ISR;

// Called when an event occurs. If less than DEBOUNCE_PERIOD_MS have elapsed
// since the event was last recorded, sets *event to true. Otherwise does nothing.
static void SetFlagIfDebounceExpired(uint32_t *lastIsrTime, __IO bool *event)
{
	bool expired = false;
	uint32_t currentTime = HAL_GetTick();

	lastActivity = currentTime;

	// Is this the first interrupt?
	if (*lastIsrTime == NO_PREV_ISR) {
		expired = true;
	}
	// Has current time wrapped around?
	else if (currentTime < *lastIsrTime) {
		expired = true;
	}
	// Has enough time passed since the last interrupt?
	else if (currentTime - *lastIsrTime > DEBOUNCE_PERIOD_MS) {
		expired = true;
	}

	if (expired) {
		*lastIsrTime = currentTime;
		*event = true;
	}
}

static __IO bool dispenseButtonPressed = false;
static uint32_t dispenseIsrTime = NO_PREV_ISR;

static __IO bool restockButtonPressed = false;
static uint32_t restockIsrTime = NO_PREV_ISR;

static __IO bool wakeupSignalReceived = false;
static uint32_t wakeupIsrTime = NO_PREV_ISR;

// Handle button press or wakeup interrupt.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin) {
	case DISPENSE_Pin:
		SetFlagIfDebounceExpired(&dispenseIsrTime, &dispenseButtonPressed);
		break;
	case RESTOCK_Pin:
		SetFlagIfDebounceExpired(&restockIsrTime, &restockButtonPressed);
		break;
	case MT3620_TO_MCU_WAKEUP_Pin:
		SetFlagIfDebounceExpired(&wakeupIsrTime, &wakeupSignalReceived);
		break;
	default:
		Error_Handler();
		break;
	}
}

// Called from non-interrupt context to check whether any buttons have
// been pressed.
void HandleButtonPress(void)
{
	int availUnits = state.stockedDispenses - state.issuedDispenses;

	if (dispenseButtonPressed) {
		if (availUnits == 0) {
			// Cannot dispense because no stock.
		} else {
			// If require restock then wake up the MT3620. This only happens when the
			// inventory passes the threshold. It does not happen for every subsequent dispense.
			if (availUnits - 1 == state.alertThreshold) {
				WakeUpMT3620();
			}

			++state.issuedDispenses;
			// In a real application, would not necessarily write to flash for every update.
			// This statement is included to demonstrate how the storage mechanism works.
			WriteLatestMachineState();
		}

		dispenseButtonPressed = false;
	}

	if (restockButtonPressed) {
		int unitsToAdd = state.machineCapacity - availUnits;

		state.stockedDispenses += unitsToAdd;
		restockButtonPressed = false;

		// In a real application, would not necessarily write to flash for every update.
		// This statement is included to demonstrate how the storage mechanism works.
		if (unitsToAdd > 0) {
			WriteLatestMachineState();
		}

		WakeUpMT3620();
	}
}

void HandleWakeupFromMT3620(void)
{
	if (wakeupSignalReceived) {
		// The MT3620 has driven the GPIO low to indicate the MCU should wake up.
		wakeupSignalReceived = false;
	}
}

// Whether the wakeup GPIO is being held high.
static bool mt3620BeingWokenUp = false;
// If mt3620BeingWokenUp is true, when the wakeup GPIO was pulled high.
static uint32_t mt3620WakeUpEndTime;

static void WakeUpMT3620(void)
{
	if (mt3620BeingWokenUp) {
		return;
	}

	// Pull GPIO line low for at least 1ms to wake up MT3620.
	HAL_GPIO_WritePin(MCU_TO_MT3620_WAKEUP_GPIO_Port, MCU_TO_MT3620_WAKEUP_Pin, GPIO_PIN_RESET);

	uint32_t currentTime = HAL_GetTick();
	mt3620WakeUpEndTime = currentTime + TO_MT3620_WAKEUP_PERIOD_MS;
	mt3620BeingWokenUp = true;
}

// Called from non-interrupt context to stop waking up the MT3620 if required.
void StopWakingUpMT3620(void)
{
	if (! mt3620BeingWokenUp) {
		return;
	}

	uint32_t currentTime = HAL_GetTick();
	if (currentTime <= mt3620WakeUpEndTime) {
		return;
	}

	// Pull GPIO line back high. MT3620 should be restarting by now.
	HAL_GPIO_WritePin(MCU_TO_MT3620_WAKEUP_GPIO_Port, MCU_TO_MT3620_WAKEUP_Pin, GPIO_PIN_SET);
	mt3620BeingWokenUp = false;
}
