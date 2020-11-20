/**************************************************************
 *
 * Servo control library for Azure Sphere SDK based devices.
 *
 * author: Balázs Simon
 *
 **************************************************************/

#ifndef SERVO_H
#define SERVO_H

struct _SERVO_State;

struct SERVO_Config
{
	int minAngleDeg;
	int maxAngleDeg;
	int pwmFd;
	unsigned int pwmChannel;
	unsigned int periodNs;
	unsigned int minPulseNs;
	unsigned int maxPulseNs;
};

extern int SERVO_Init(struct SERVO_Config* config, struct _SERVO_State** state);

extern int SERVO_SetAngle(struct _SERVO_State* servoState, float angle);

extern int SERVO_Destroy(struct _SERVO_State* servoState);

/// <summary>Servo resting angle</summary>
#define SERVO_DISABLED_ANGLE 115

/// <summary>Servo resting angle</summary>
#define SERVO_STANDBY_ANGLE 0

/// <summary>Servo min angle</summary>
#define SERVO_MIN_ANGLE 0

/// <summary>Servo max angle</summary>
#define SERVO_MAX_ANGLE 180

/// <summary>Servo converging speed</summary>
#define SERVO_CONVERGING_SPEED 500.0f

#endif
