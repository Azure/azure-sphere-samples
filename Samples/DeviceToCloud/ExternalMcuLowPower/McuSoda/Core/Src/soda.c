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
	SetFlavorLedEnabled(true);

	nextSleepTimeTicks = HAL_GetTick();

	ReadMessageAsync();
	for (;;) {
		HAL_SuspendTick();
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		HAL_ResumeTick();

		uint32_t now;

		// A do loop, rather than a while loop, is used because the RX and TX interrupts
		// do not adjust nextSleepTimeTicks.
		do
		{
			now = HAL_GetTick();

			HandleWakeupFromMT3620();
			StopWakingUpMT3620(now);

			StopBlinkingFlavorLed(now);
			HandleButtonPress();
			HandleMessage();

			// Sleep until the next tick or other interrupt.
			HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

			// The system may have been woken up by incoming serial data from
			// the MT3620. In that case, need to check again for a completed message
			// because the handler would not have updated nextSleepTimeTicks.
			HandleMessage();
		} while (nextSleepTimeTicks >= now);
	}
}

