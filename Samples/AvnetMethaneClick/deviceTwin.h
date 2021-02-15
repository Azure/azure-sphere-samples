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

// Constants
#define JSON_BUFFER_SIZE 512
#define CLOUD_MSG_SIZE 22
#define MAX_DEVICE_TWIN_PAYLOAD_SIZE 1024 + 512

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

typedef struct {
	char* twinKey;
	void* twinVar;
	int* twinFd;
	GPIO_Id twinGPIO;
	data_type_t twinType;
	bool active_high;
} twin_t;

extern twin_t twinArray[];
extern int twinArraySize;
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;
extern void ReportedStateCallback(int, void*);
extern void TwinReportState(const char*);
extern void CloseFdAndPrintError(int, const char*);
extern void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback);
extern volatile sig_atomic_t exitCode;

int desiredVersion;

void checkAndUpdateDeviceTwin(char*, void*, data_type_t, bool);
void sendInitialDeviceTwinReportedProperties(void);
void deviceTwinOpenFDs(void);
void deviceTwinCloseFDs(void);


#define NO_GPIO_ASSOCIATED_WITH_TWIN -1

#endif // C_DEVICE_TWIN_H