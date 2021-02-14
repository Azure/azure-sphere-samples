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
extern volatile sig_atomic_t exitCode;

int desiredVersion;

// Declare any device twin handlers here
void genericIntDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericFloatDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericBoolDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericGPIODTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);
void genericStringDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties);

void checkAndUpdateDeviceTwin(char*, void*, data_type_t, bool);
void sendInitialDeviceTwinReportedProperties(void);
void deviceTwinOpenFDs(void);
void deviceTwinCloseFDs(void);


#define NO_GPIO_ASSOCIATED_WITH_TWIN -1

#endif // C_DEVICE_TWIN_H