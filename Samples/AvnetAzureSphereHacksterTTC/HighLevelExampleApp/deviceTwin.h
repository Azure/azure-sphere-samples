/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef C_DEVICE_TWIN_H
#define C_DEVICE_TWIN_H

#include <applibs/gpio.h>
#include "parson.h"
#include "signal.h"
#include "build_options.h"

// Azure IoT SDK
#include <azure_sphere_provisioning.h>

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <hw/sample_appliance.h>
#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"
#include "parson.h"
#include "exit_codes.h"
#include "build_options.h"

// Constants
#define JSON_BUFFER_SIZE 512
#define CLOUD_MSG_SIZE 22
#define MAX_DEVICE_TWIN_PAYLOAD_SIZE 2048

// Define the device twin reported property JSON format for different data types
static const char cstrDeviceTwinJsonInteger[] = "{\"%s\": %d}";
static const char cstrDeviceTwinJsonFloat[] = "{\"%s\": %.2f}";
static const char cstrDeviceTwinJsonBool[] = "{\"%s\": %s}";
static const char cstrDeviceTwinJsonString[] = "{\"%s\": \"%s\"}";

#ifdef USE_PNP
// See https://docs.microsoft.com/en-us/azure/iot-pnp/concepts-developer-guide-device?pivots=programming-language-ansi-c
// for PnP formatting/data requirements
static const char cstrDeviceTwinPnPJsonInteger[] = "{\"%s\":{\"value\":%.d,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";
static const char cstrDeviceTwinPnPJsonFloat[] = "{\"%s\":{\"value\":%.2f,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";
static const char cstrDeviceTwinPnPJsonBool[] = "{\"%s\":{\"value\":%s,\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";
static const char cstrDeviceTwinPnPJsonString[] = "{\"%s\":{\"value\":\"%s\",\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}";
#endif 

typedef enum {
	TYPE_INT = 0,
	TYPE_FLOAT = 1,
	TYPE_BOOL = 2,
	TYPE_STRING = 3
} data_type_t;

// Define the signature for the device twin function pointer
// We need to use the void* to avoid a cicular reference in the twin_t struct
typedef void (*dtHandler)(void*, JSON_Object*);

typedef struct {
	char* twinKey;
	void* twinVar;
	int* twinFd;
	GPIO_Id twinGPIO;
	data_type_t twinType;
	bool active_high;
	dtHandler twinHandler;
} twin_t;


extern twin_t twinArray[];
extern int twinArraySize;
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;
extern void ReportedStateCallback(int, void*);
extern void TwinReportState(const char*);
extern void CloseFdAndPrintError(int, const char*);
extern void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback);
// Variables used to update the polling time between sending telemetry data
extern EventLoopTimer *sensorPollTimer;
extern int readSensorPeriod;

extern volatile sig_atomic_t exitCode;

int desiredVersion;

// Declare any device twin handlers here
void genericIntDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericFloatDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericBoolDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericGPIODTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericStringDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);

// Custom handler for poll timer
void setSensorPollTimerFunction(void* thisTwinPtr, JSON_Object *desiredProperties);

void checkAndUpdateDeviceTwin(char*, void*, data_type_t, bool);
void sendInitialDeviceTwinReportedProperties(void);
void deviceTwinOpenFDs(void);
void deviceTwinCloseFDs(void);


#define NO_GPIO_ASSOCIATED_WITH_TWIN -1

#endif // C_DEVICE_TWIN_H