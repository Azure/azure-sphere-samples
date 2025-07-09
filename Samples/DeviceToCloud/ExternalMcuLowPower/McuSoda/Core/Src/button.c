/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

static void SetNextSleepTime(uint32_t candidateEndTime);
static void SetNextSleepTimeFromNow(uint32_t *absTicks, uint32_t deltaTicks);

static void SetFlagIfDebounceExpired(uint32_t *lastIsrTime, __IO bool *event);
static void StartBlinkingFlavorLed(void);
static void WakeUpMT3620(void);

// The next time in ticks when the device can go to sleep.
__IO uint32_t nextSleepTimeTicks;

// If the supplied candidate end time is greater than the current
// end time, then extend the current end time.
static void SetNextSleepTime(uint32_t candidateEndTime)
{
	if (candidateEndTime > nextSleepTimeTicks) {
		nextSleepTimeTicks = candidateEndTime;
	}
}

static void SetNextSleepTimeFromNow(uint32_t *absTicks, uint32_t deltaTicks)
{
	uint32_t now = HAL_GetTick();
	uint32_t candidateTime = now + deltaTicks;
	if (absTicks != NULL) {
		*absTicks = candidateTime;
	}

	// Allow an extra tick so the handlers can check for <= end_time
	// to avoid rounding down the required amount of time.
	SetNextSleepTime(candidateTime + 1);
}

#define NO_PREV_ISR			0xFFFFFFFF
// The last time when a button or wakeup occurred.
static __IO uint32_t lastActivityTicks = NO_PREV_ISR;

#define DEBOUNCE_PERIOD_MS	250

// Called when an interrupt occurs.
//
// If *event is false on entry, and the event has not occurred before
// (*lastIsrTime == NO_PREV_ISR), sets *lastIsrTime to the current time,
// and *event to true.
//
// If *event is false on entry, but DEBOUNCE_PERIOD_MS milliseconds or
// less have elapsed since the event last occurred, this function does nothing
// because still in the debounce period.
//
// If *event if false on entry, and more than DEBOUNCE_PERIOD_MS have elapsed
// since the event last occurred, then sets *lastIsrTime to the current time
// and *event to true.
static void SetFlagIfDebounceExpired(uint32_t *lastIsrTime, __IO bool *event)
{
	bool expired = false;
	uint32_t now = HAL_GetTick();

	lastActivityTicks = now;

	// Do nothing if the event is already in progress.
	if (*event) {
		// empty.
	}

	// Is this the first interrupt?
	else if (*lastIsrTime == NO_PREV_ISR) {
		expired = true;
	}

	// Has current time wrapped around? If so, assume the debounce period
	// has expired.
	else if (now < *lastIsrTime) {
		expired = true;
	}

	// Has enough time passed since the last interrupt?
	else if (now - *lastIsrTime >= DEBOUNCE_PERIOD_MS) {
		expired = true;
	}

	// A new event has occurred, so start a new debounce period.
	if (expired) {
		*lastIsrTime = now;
		SetNextSleepTimeFromNow(NULL, DEBOUNCE_PERIOD_MS);
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
		dispenseButtonPressed = false;

		// If require restock then wake up the MT3620.
		if (availUnits - 1 <= state.alertThreshold) {
			WakeUpMT3620();
		}

		if (availUnits == 0) {
			// Cannot dispense because no stock.
		} else {
			++state.issuedDispenses;
			// In a real application, would not necessarily write to flash for every update.
			// This statement is included to demonstrate how the storage mechanism works.
			WriteLatestMachineState();
			StartBlinkingFlavorLed();
		}
	}

	if (restockButtonPressed) {
		restockButtonPressed = false;

		int unitsToAdd = state.machineCapacity - availUnits;

		if (unitsToAdd > 0) {
			state.stockedDispenses += unitsToAdd;

			// In a real application, would not necessarily write to flash for every update.
			// This statement is included to demonstrate how the storage mechanism works.

			WriteLatestMachineState();
			WakeUpMT3620();
		}
	}
}

void HandleWakeupFromMT3620(void)
{
	if (wakeupSignalReceived) {
		// The MT3620 has driven the GPIO low to indicate the MCU should wake up.
		wakeupSignalReceived = false;
	}
}

// ---- Dispense LED ----

static bool blinkingDispenseLed = false;
static uint32_t blinkEndTime;

// Last blink state - false = off, true = on.
static bool lastEnabledState;
// Last time the LED state was changed.
static int lastTransitionTime;

// Blink the flavor LED for DISPENSE_LED_PERIOD_MS.
// If the LED is already blinking because of a previous dispense,
// then the blink period ends DISPENSE_LED_PERIOD_MS from now.
static void StartBlinkingFlavorLed(void)
{
	if (! blinkingDispenseLed) {
		SetFlavorLedEnabled(false);
		lastEnabledState = false;
		lastTransitionTime = HAL_GetTick();
	}

	blinkingDispenseLed = true;
	SetNextSleepTimeFromNow(&blinkEndTime, DISPENSE_LED_PERIOD_MS);
}

// Called in non-interrupt context to switch off the dispense LED
// if it is currently switched on and enough time has passed.
void StopBlinkingFlavorLed(uint32_t now)
{
	if (! blinkingDispenseLed) {
		return;
	}

	// If in middle of blink period, then toggle LED every 0.5 seconds.
	// This function will be called at least once after blinkEndTime to
	// reset the LED.
	if (blinkEndTime >= now) {
		if ((now - lastTransitionTime) > 500) {
			lastEnabledState = ! lastEnabledState;
			SetFlavorLedEnabled(lastEnabledState);
			lastTransitionTime = now;
		}
		return;
	}

	// Finished blink period so ensure LED is left on.
	SetFlavorLedEnabled(true);
	blinkingDispenseLed = false;
}

// ---- Wake up MT3620 ----

// Whether the wakeup GPIO is being held high.
static bool mt3620BeingWokenUp = false;
// If mt3620BeingWokenUp is true, when the wakeup GPIO was pulled high.
static uint32_t mt3620WakeupEndTime = 0xFFFFFFFF;

// Called from non-interrupt context to wake up the MT3620 if required.
static void WakeUpMT3620(void)
{
	// Pull GPIO line low for at least 1ms to wake up MT3620.
	HAL_GPIO_WritePin(MCU_TO_MT3620_WAKEUP_GPIO_Port, MCU_TO_MT3620_WAKEUP_Pin, GPIO_PIN_RESET);

	SetNextSleepTimeFromNow(&mt3620WakeupEndTime, TO_MT3620_WAKEUP_PERIOD_MS);
	mt3620BeingWokenUp = true;
}

// Called from non-interrupt context to stop waking up the MT3620 if required.
void StopWakingUpMT3620(uint32_t now)
{
	if (! mt3620BeingWokenUp) {
		return;
	}

	// If not yet ready to stop waking up the MT3620, then return.
	// The wakeup line is pulled high after the end time.
	if (now <= mt3620WakeupEndTime) {
		return;
	}

	// Pull GPIO line back high. MT3620 should be restarting by now.
	HAL_GPIO_WritePin(MCU_TO_MT3620_WAKEUP_GPIO_Port, MCU_TO_MT3620_WAKEUP_Pin, GPIO_PIN_SET);
	mt3620BeingWokenUp = false;
}
