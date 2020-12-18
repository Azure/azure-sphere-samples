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
#include "deviceTwin.h"
#include "parson.h"
#include "exit_codes.h"
#include "build_options.h"

bool userLedRedIsOn = false;
bool userLedGreenIsOn = false;
bool userLedBlueIsOn = false;
bool appLedIsOn = false;
bool wifiLedIsOn = false;
bool clkBoardRelay1IsOn = true;
bool clkBoardRelay2IsOn = true;

uint8_t oled_ms1[CLOUD_MSG_SIZE] = "    Azure Sphere";
uint8_t oled_ms2[CLOUD_MSG_SIZE];
uint8_t oled_ms3[CLOUD_MSG_SIZE] = "    Avnet MT3620";
uint8_t oled_ms4[CLOUD_MSG_SIZE] = "    Starter Kit";

extern int userLedRedFd;
extern int userLedGreenFd;
extern int userLedBlueFd;

extern int wifiLedFd;
extern int appLedFd;
extern int clickSocket1Relay1Fd;
extern int clickSocket1Relay2Fd;

extern volatile sig_atomic_t terminationRequired;

// Track the current device twin version.  This is updated when we receive a device twin
// update, and used when we send a device twin reported property
int desiredVersion = 0;

// Define each device twin key that we plan to catch, process, and send reported property for.
// .twinKey - The JSON Key piece of the key: value pair
// .twinVar - The address of the application variable keep this key: value pair data
// .twinFD - The associated File Descriptor for this item.  This is usually a GPIO FD.  NULL if NA.
// .twinGPIO - The associted GPIO number for this item.  NO_GPIO_ASSOCIATED_WITH_TWIN if NA
// .twinType - The data type for this item, TYPE_BOOL, TYPE_STRING, TYPE_INT, or TYPE_FLOAT
// .active_high - true if GPIO item is active high, false if active low.  This is used to init the GPIO 
twin_t twinArray[] = {
	{.twinKey = "userLedRed",.twinVar = &userLedRedIsOn,.twinFd = &userLedRedFd,.twinGPIO = SAMPLE_RGBLED_RED,.twinType = TYPE_BOOL,.active_high = false},
	{.twinKey = "userLedGreen",.twinVar = &userLedGreenIsOn,.twinFd = &userLedGreenFd,.twinGPIO = SAMPLE_RGBLED_GREEN,.twinType = TYPE_BOOL,.active_high = false},
	{.twinKey = "userLedBlue",.twinVar = &userLedBlueIsOn,.twinFd = &userLedBlueFd,.twinGPIO = SAMPLE_RGBLED_BLUE,.twinType = TYPE_BOOL,.active_high = false},
	{.twinKey = "wifiLed",.twinVar = &wifiLedIsOn,.twinFd = &wifiLedFd,.twinGPIO = SAMPLE_WIFI_LED,.twinType = TYPE_BOOL,.active_high = false},
	{.twinKey = "appLed",.twinVar = &appLedIsOn,.twinFd = &appLedFd,.twinGPIO = SAMPLE_APP_LED,.twinType = TYPE_BOOL,.active_high = false},
	{.twinKey = "clickBoardRelay1",.twinVar = &clkBoardRelay1IsOn,.twinFd = &clickSocket1Relay1Fd,.twinGPIO = RELAY_CLICK_RELAY1,.twinType = TYPE_BOOL,.active_high = true},
	{.twinKey = "clickBoardRelay2",.twinVar = &clkBoardRelay2IsOn,.twinFd = &clickSocket1Relay2Fd,.twinGPIO = RELAY_CLICK_RELAY2,.twinType = TYPE_BOOL,.active_high = true},
	{.twinKey = "OledDisplayMsg1",.twinVar = oled_ms1,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_STRING,.active_high = true},
	{.twinKey = "OledDisplayMsg2",.twinVar = oled_ms2,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_STRING,.active_high = true},
	{.twinKey = "OledDisplayMsg3",.twinVar = oled_ms3,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_STRING,.active_high = true},
	{.twinKey = "OledDisplayMsg4",.twinVar = oled_ms4,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_STRING,.active_high = true}
};

// Calculate how many twin_t items are in the array.  We use this to iterate through the structure.
int twinArraySize = sizeof(twinArray) / sizeof(twin_t);

///<summary>
///		check to see if any of the device twin properties have been updated.  If so, send up the current data.
///</summary>
void checkAndUpdateDeviceTwin(char* property, void* value, data_type_t type, bool ioTPnPFormat)
{
	int nJsonLength = -1;
#ifdef USE_PNP
	char* resultTxt = "Property successfully updated";
#endif 

	char *pjsonBuffer = (char *)malloc(JSON_BUFFER_SIZE);
	if (pjsonBuffer == NULL) {
		Log_Debug("ERROR: not enough memory to report device twin changes.");
	}

	if (property != NULL) {

		// report current device twin data as reported properties to IoTHub

		switch (type) {
		case TYPE_BOOL:
#ifdef USE_PNP     
			if(ioTPnPFormat){
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinPnPJsonBool, property, *(bool*)value ? "true" : "false", 200, desiredVersion, resultTxt);	
			}
            else
#endif 
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonBool, property, *(bool*)value ? "true" : "false", desiredVersion);
			break;
		case TYPE_FLOAT:
#ifdef USE_PNP     			
            if(ioTPnPFormat){
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinPnPJsonFloat, property, *(float*)value, 200, desiredVersion, resultTxt);	
			}
            else
#endif 
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonFloat, property, *(float*)value, desiredVersion);
			break;
		case TYPE_INT:
#ifdef USE_PNP     						
			if(ioTPnPFormat){
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinPnPJsonInteger, property, *(int*)value, 200, desiredVersion, resultTxt);	
			}
            else
#endif 
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonInteger, property, *(int*)value, desiredVersion);
			break;
 		case TYPE_STRING:
#ifdef USE_PNP     					
        	if(ioTPnPFormat){
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinPnPJsonString, property, (char*)value, 200, desiredVersion, resultTxt);	
			}
            else
#endif             
				nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonString, property, (char*)value, desiredVersion);
			break;
		}

		if (nJsonLength > 0) {
			Log_Debug("[MCU] Updating device twin: %s\n", pjsonBuffer);
            TwinReportState(pjsonBuffer);
		}
		free(pjsonBuffer);
	}
}

/// <summary>
///     Callback invoked when a Device Twin update is received from Azure IoT Hub.
/// </summary>
void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback)
{
    // Statically allocate this for more predictable memory use patterns
    static char nullTerminatedJsonString[MAX_DEVICE_TWIN_PAYLOAD_SIZE + 1];

    if (payloadSize > MAX_DEVICE_TWIN_PAYLOAD_SIZE) {
        Log_Debug("ERROR: Device twin payload size (%u bytes) exceeds maximum (%u bytes).\n",
                  payloadSize, MAX_DEVICE_TWIN_PAYLOAD_SIZE);

        exitCode = ExitCode_PayloadSize_TooLarge;
        return;
    }

    // Copy the payload to local buffer for null-termination.
    memcpy(nullTerminatedJsonString, payload, payloadSize);

    // Add the null terminator at the end.
    nullTerminatedJsonString[payloadSize] = 0;

    // Parse the payload and prepare it for the parson parser
    JSON_Value *rootProperties = NULL;
    rootProperties = json_parse_string(nullTerminatedJsonString);
    if (rootProperties == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
        goto cleanup;
    }

    JSON_Object *rootObject = json_value_get_object(rootProperties);
    JSON_Object *desiredProperties = json_object_dotget_object(rootObject, "desired");
    if (desiredProperties == NULL) {
        desiredProperties = rootObject;
    }

    // Pull the twin version out of the message.  We use this value when we echo the new setting
    // back to IoT Connect.
    if (json_object_has_value(desiredProperties, "$version") != 0) {
        desiredVersion = (int)json_object_get_number(desiredProperties, "$version");
    }

	int result = 0;
    for (int i = 0; i < twinArraySize; i++)
    {

        if (json_object_has_value(desiredProperties, twinArray[i].twinKey) != 0) {

            switch (twinArray[i].twinType) {
            case TYPE_BOOL:
                *(bool *)twinArray[i].twinVar =
                    (bool)json_object_get_boolean(desiredProperties, twinArray[i].twinKey);
                result = GPIO_SetValue(*twinArray[i].twinFd,
                                       twinArray[i].active_high
                                           ? (GPIO_Value) * (bool *)twinArray[i].twinVar
                                           : !(GPIO_Value) * (bool *)twinArray[i].twinVar);

                if (result != 0) {
                    Log_Debug("Fd: %d\n", twinArray[i].twinFd);
                    Log_Debug("FAILURE: Could not set GPIO_%d, %d output value %d: %s (%d).\n",
                              twinArray[i].twinGPIO, twinArray[i].twinFd,
                              (GPIO_Value) * (bool *)twinArray[i].twinVar, strerror(errno), errno);
                    exitCode = ExitCode_SetGPIO_Failed;
                }
                Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey,
                          *(bool *)twinArray[i].twinVar ? "true" : "false");
                checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_BOOL,
                                         true);
                break;
            case TYPE_FLOAT:
                *(float *)twinArray[i].twinVar =
                    (float)json_object_get_number(desiredProperties, twinArray[i].twinKey);
                Log_Debug("Received device update. New %s is %0.2f\n", twinArray[i].twinKey,
                          *(float *)twinArray[i].twinVar);
                checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_FLOAT,
                                         true);
                break;
            case TYPE_INT:
                *(int *)twinArray[i].twinVar =
                    (int)json_object_get_number(desiredProperties, twinArray[i].twinKey);
                Log_Debug("Received device update. New %s is %d\n", twinArray[i].twinKey,
                          *(int *)twinArray[i].twinVar);
                checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_INT,
                                         true);
                break;

            case TYPE_STRING:
                strcpy((char *)twinArray[i].twinVar,
                       (char *)json_object_get_string(desiredProperties, twinArray[i].twinKey));
                Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey,
                          (char *)twinArray[i].twinVar);
                checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_STRING,
                                         true);
                break;
            }
        }
    }

cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
}

void sendInitialDeviceTwinReportedProperties(void)
{
    for (int i = 0; i < (sizeof(twinArray) / sizeof(twin_t)); i++) {

        switch (twinArray[i].twinType) {
        case TYPE_BOOL:
            Log_Debug("Send twin update. New %s is %s\n", twinArray[i].twinKey,
                      *(bool *)twinArray[i].twinVar ? "true" : "false");
            checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_BOOL,
                                     true);
            break;
        case TYPE_FLOAT:
            Log_Debug("Received device update. New %s is %0.2f\n", twinArray[i].twinKey,
                      *(float *)twinArray[i].twinVar);
            checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_FLOAT,
                                     true);
            break;
        case TYPE_INT:
            Log_Debug("Received device update. New %s is %d\n", twinArray[i].twinKey,
                      *(int *)twinArray[i].twinVar);
            checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_INT,
                                     true);
            break;

        case TYPE_STRING:
            Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey,
                      (char *)twinArray[i].twinVar);
            checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_STRING,
                                     true);
            break;
        }
    }
}

void deviceTwinOpenFDs()
{

    // Traverse the twin Array and for each GPIO item in the list open the file descriptor
    for (int i = 0; i < twinArraySize; i++) {

        // Verify that this entry is a GPIO entry
        if (twinArray[i].twinGPIO != NO_GPIO_ASSOCIATED_WITH_TWIN) {

            *twinArray[i].twinFd = -1;

            // For each item in the data structure, initialize the file descriptor and open the GPIO
            // for output.  Initilize each GPIO to its specific inactive state.
            *twinArray[i].twinFd =
                (int)GPIO_OpenAsOutput(twinArray[i].twinGPIO, GPIO_OutputMode_PushPull,
                                       twinArray[i].active_high ? GPIO_Value_Low : GPIO_Value_High);

            if (*twinArray[i].twinFd < 0) {
                Log_Debug("ERROR: Could not open LED %d: %s (%d).\n", twinArray[i].twinGPIO,
                          strerror(errno), errno);
            }
        }
    }
}

void deviceTwinCloseFDs(void)
{
    // Traverse the twin Array and for each GPIO item in the list the close the file descriptor
    for (int i = 0; i < twinArraySize; i++) {

        // Verify that this entry has an open file descriptor
        if (twinArray[i].twinGPIO != NO_GPIO_ASSOCIATED_WITH_TWIN) {

            CloseFdAndPrintError(*twinArray[i].twinFd, twinArray[i].twinKey);
        }
    }
}
