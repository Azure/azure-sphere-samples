/**************************************************************
 *
 * Servo control library for Azure Sphere SDK based devices.
 *
 * author: Balázs Simon
 *
 **************************************************************/

#include "servo.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <applibs/log.h>
#include <applibs/pwm.h>

struct _SERVO_State
{
	struct PwmState* pwmState;
	int pwmFd;
	int minAngleDeg;
	int maxAngleDeg;
	unsigned int pwmChannel;
	unsigned int minPulseMs;
	unsigned int maxPulseMs;
};

int SERVO_Init(struct SERVO_Config* config, struct _SERVO_State** state)
{
	// Check for an invalid configuration
	if ((NULL == config) || (NULL == state) ||
		(config->maxAngleDeg <= config->minAngleDeg) ||
		(config->maxPulseNs <= config->minPulseNs) ||
		(config->periodNs <= config->maxPulseNs) ||
		(config->pwmFd < 0))
	{
		errno = EINVAL;
		*state = NULL;
		Log_Debug("SERVO_Init failed: invalid config or state, errno: %s (%d)\n", strerror(errno));
		return -1;
	}

	// Allocate memory for a new structure
	*state = (struct _SERVO_State*)malloc(sizeof(struct _SERVO_State));

	// Copy the new configuration into the new structure
	(*state)->pwmFd = config->pwmFd;
	(*state)->pwmChannel = config->pwmChannel;
	(*state)->minPulseMs = config->minPulseNs;
	(*state)->maxPulseMs = config->maxPulseNs;
	(*state)->minAngleDeg = config->minAngleDeg;
	(*state)->maxAngleDeg = config->maxAngleDeg;
	(*state)->pwmState = (struct PwmState*)malloc(sizeof(struct PwmState));
	(*state)->pwmState->period_nsec = config->periodNs;
	(*state)->pwmState->polarity = PWM_Polarity_Normal;
	(*state)->pwmState->dutyCycle_nsec = 0;
	(*state)->pwmState->enabled = true;

	return 0;
}

int SERVO_SetAngle(struct _SERVO_State* servo, float angle)
{
	// Check for a valid structure
	if (NULL == servo)
	{
		errno = EINVAL;
		return -1;
	}

	float pulsePercent;

	// Calculate the pulse percentage
	if (angle > servo->maxAngleDeg) { pulsePercent = 1.0f; }
	else if (angle < servo->minAngleDeg) { pulsePercent = 0.0f; } else {
            pulsePercent = (float)(angle - (float)servo->minAngleDeg) /
                           (float)(servo->maxAngleDeg - servo->minAngleDeg);
        }

	// Calculate the duty cycle in nanoSeconds
	servo->pwmState->dutyCycle_nsec =
            (unsigned int)((float)(servo->maxPulseMs - servo->minPulseMs) * pulsePercent +
            (float)servo->minPulseMs);

	// Apply the change to the hardware interface
	int result = PWM_Apply(servo->pwmFd, servo->pwmChannel, servo->pwmState);
	if (result != 0) {
		Log_Debug("PWM_Apply failed: result = %d, errno: %s (%d)\n", result, strerror(errno),
			errno);
		return -1;
	}

	return 0;
}

int SERVO_Destroy(struct _SERVO_State* servo)
{
	// Free the memory allocated by the init call
	free(servo->pwmState);
    free(servo);
	return 0;
}