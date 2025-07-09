
#include "main.h"

static bool red = false;
static bool green = false;
static bool blue = false;

static bool ledOn = true;

static void UpdateLedStatus(void);

// Update the LED color. If the flavor LED is currently switched off, then
// it will not be updated until the next time it is switched on.
void SetFlavor(bool r, bool g, bool b)
{
	red = r;
	green = g;
	blue = b;

	UpdateLedStatus();
}

void SetFlavorLedEnabled(bool enabled)
{
	ledOn = enabled;
	UpdateLedStatus();
}

static void UpdateLedStatus(void)
{
	// Please note that the code below presumes a common-anode RGB LED.
	// If using a common-cathode, you will need to swap GPIO_PIN_SET and GPIO_PIN_RESET.

	static const GPIO_PinState ledOnState = GPIO_PIN_RESET;
	static const GPIO_PinState ledOffState = GPIO_PIN_SET;

	if (ledOn) {
		HAL_GPIO_WritePin(TRILED_R_GPIO_Port, TRILED_R_Pin, red ? ledOnState : ledOffState);
		HAL_GPIO_WritePin(TRILED_G_GPIO_Port, TRILED_G_Pin, green ? ledOnState : ledOffState);
		HAL_GPIO_WritePin(TRILED_B_GPIO_Port, TRILED_B_Pin, blue ? ledOnState : ledOffState);
	} else {
		HAL_GPIO_WritePin(TRILED_R_GPIO_Port, TRILED_R_Pin, ledOffState);
		HAL_GPIO_WritePin(TRILED_G_GPIO_Port, TRILED_G_Pin, ledOffState);
		HAL_GPIO_WritePin(TRILED_B_GPIO_Port, TRILED_B_Pin, ledOffState);
	}
}
