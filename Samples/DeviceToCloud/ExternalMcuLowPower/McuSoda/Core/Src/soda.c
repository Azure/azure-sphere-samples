/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "configuration.h"
#include "main.h"

// Initial machine state. The first time this program is run this state
// is written to NVM. Thereafter, the initial state is loaded from NVM.
MachineState state = {
	.machineCapacity = MachineCapacity,
	.alertThreshold = LowDispenseAlertThreshold,
	.stockedDispenses = 0,
	.issuedDispenses = 0
};

// Infinite loop waits for and then handled external events, viz. button press,
// wakeup, and UART RX.
_Noreturn void RunSodaMachine(void)
{
	RestoreStateFromFlash();

	ReadMessageAsync();
	for (;;) {
		HAL_SuspendTick();
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		HAL_ResumeTick();

		// The system tick is stopped during WFI, so don't sleep if still in
		// a debounce period because when the device wakes because of an interrupt,
		// the handler will think it is still in the debounce period.

		while (lastActivity == NO_PREV_ISR || HAL_GetTick() < lastActivity + (2 * DEBOUNCE_PERIOD_MS)) {
			HandleWakeupFromMT3620();
			StopWakingUpMT3620();
			HandleButtonPress();
			HandleMessage();
		}
	}
}

