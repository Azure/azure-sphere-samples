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
#include "device_twin.h"
#include "parson.h"
#include "../common/exitcodes.h"
#include "build_options.h"
#include "m4_support.h"

#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"

// Variabled to operate on Starter Kit application LED
bool appLedIsOn = false;
int appLedFd = -1;

#ifdef IOT_HUB_APPLICATION

extern volatile sig_atomic_t terminationRequired;
int sendTelemetryPeriod = SEND_TELEMETRY_PERIOD_SECONDS;


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
// .twinHandler - The handler that will be called for this device twin.  The function must have the signaure 
//                void <yourFunctionName>(void* thisTwinPtr, JSON_Object *desiredProperties);
// 
twin_t twinArray[] = {
    {.twinKey = "sensorPollPeriod",.twinVar = &readSensorPeriod,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_INT,.active_high = true,.twinHandler = (setSensorPollTimerFunction)},   
    {.twinKey = "telemetryPeriod",.twinVar = &sendTelemetryPeriod,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_INT,.active_high = true,.twinHandler = (setTelemetryTimerFunction)},
#ifdef M4_INTERCORE_COMMS
    {.twinKey = "realTimeAutoTelemetryPeriod",.twinVar = &realTimeAutoTelemetryInterval,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_INT,.active_high = true,.twinHandler = (setRealTimeTelemetryInterval)},
#endif     
};

// Calculate how many twin_t items are in the array.  We use this to iterate through the structure.
int twinArraySize = sizeof(twinArray) / sizeof(twin_t);

///<summary>
///		Generic device twin handler for Integer types
///     This handler will update the integer variable referenced in the device twin handler
///</summary>
void genericIntDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Updte the variable referenced in the twin table
    *(int *)(twin_t*)localTwinPtr->twinVar = (int)json_object_get_number(desiredProperties, localTwinPtr->twinKey);
    Log_Debug("Received device update. New %s is %d\n", localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
    
    // Send the reported property to the IoTHub
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, localTwinPtr->twinKey, localTwinPtr->twinVar);

}

///<summary>
///		Generic device twin handler for Float types
///     This handler will update the float variable referenced in the device twin handler
///</summary>
void genericFloatDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Update the variable referenced by the twin table
    *(float *)localTwinPtr->twinVar = (float)json_object_get_number(desiredProperties, localTwinPtr->twinKey);
    Log_Debug("Received device update. New %s is %0.2f\n", localTwinPtr->twinKey, *(float *)localTwinPtr->twinVar);

    // Send the reported property to the IoTHub    
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_FLOAT, localTwinPtr->twinKey, localTwinPtr->twinVar);
}

///<summary>
///		Generic device twin handler for Boolean types (no GPIO device associated)
///</summary>
void genericBoolDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Update the variable referenced by the twin table
    *(bool *)localTwinPtr->twinVar = (bool)json_object_get_boolean(desiredProperties, localTwinPtr->twinKey);
    Log_Debug("Received device update. New %s is %s\n", localTwinPtr->twinKey,
                                                       *(bool *)localTwinPtr->twinVar ? "true" : "false");

    // Send the reported proptery to the IoTHub
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_BOOL, localTwinPtr->twinKey, localTwinPtr->twinVar);
}

///<summary>
///		Generic device twin handler for Boolean types with associated GPIO
///     This handler will update the integer variable referenced in the device twin handler and also set
//      the GPIO signal using details in the twin table
///</summary>
void genericGPIODTFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Read the new boolean value from the desired property structure
    *(bool *)localTwinPtr->twinVar = (bool)json_object_get_boolean(desiredProperties, localTwinPtr->twinKey);
    
    // Set the GPIO value using details from the twin table
    int result = GPIO_SetValue(*localTwinPtr->twinFd, localTwinPtr->active_high
                                                 ? (GPIO_Value) * (bool *)localTwinPtr->twinVar
                                                 : !(GPIO_Value) * (bool *)localTwinPtr->twinVar);
    
    // Check to make sure we we able to set the GPIO signal
    if (result != 0) {
        Log_Debug("Fd: %d\n", localTwinPtr->twinFd);
        Log_Debug("FAILURE: Could not set GPIO_%d, %d output value %d: %s (%d).\n",
                    localTwinPtr->twinGPIO, localTwinPtr->twinFd,
                    (GPIO_Value) * (bool *)localTwinPtr->twinVar, strerror(errno), errno);
        exitCode = ExitCode_SetGPIO_Failed;
    }
    
    Log_Debug("Received device update. New %s is %s\n", localTwinPtr->twinKey,
                                                       *(bool *)localTwinPtr->twinVar ? "true" : "false");
    
    // Send the reported property to the IoTHub    
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_BOOL, localTwinPtr->twinKey, localTwinPtr->twinVar);
}

///<summary>
///		Generic device twin handler for String types
///     This handler will update the string variable referenced in the device twin handler
///</summary>
void genericStringDTFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // check to see if we have an empty string
    if(json_object_get_string(desiredProperties, localTwinPtr->twinKey) != NULL){                
                
        // The string is NOT empty, move the string into the local variable
        strcpy((char *)localTwinPtr->twinVar, (char *)json_object_get_string(desiredProperties, localTwinPtr->twinKey));
    }
    else {
        // The string is empty, update the local variable to ""
        strcpy((char *)localTwinPtr->twinVar, "");
    }
    
    Log_Debug("Received device update. New %s is %s\n", localTwinPtr->twinKey, (char *)localTwinPtr->twinVar);
    
    // Send the reported proptery to the IoTHub
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_STRING, localTwinPtr->twinKey, localTwinPtr->twinVar);

}

///<summary>
///		Handler to update the sensor poll timer
///     
///</summary>
void setSensorPollTimerFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

   // Read the new value
    int tempSensorPollPeriod = (int)json_object_get_number(desiredProperties, localTwinPtr->twinKey);
    
    // Make sure that the new timer variable is not zero or negitive
    if(tempSensorPollPeriod > 0){

 	    // Define a new timespec variable for the timer and change the timer period
	    struct timespec newPeriod = { .tv_sec = tempSensorPollPeriod,.tv_nsec = 0 };
        SetEventLoopTimerPeriod(sensorPollTimer, &newPeriod);

    }
    else if(tempSensorPollPeriod == 0){

        // If the new period is zero, then stop the timer
        DisarmEventLoopTimer(sensorPollTimer);
    
    }
    else{
        // The data is out of range, return without updating anything

        Log_Debug("Received invalid device update for key %s.\n", localTwinPtr->twinKey);
        // Send the reported property to the IoTHub
        updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
        
        return;
    }

    // Updte the variable referenced in the twin table
    *(int *)(twin_t*)localTwinPtr->twinVar = tempSensorPollPeriod;

    // Send the reported property to the IoTHub
    Log_Debug("Received device update. New %s is %d\n", localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
}

#ifdef M4_INTERCORE_COMMS
///<summary>
///		Custom device twin handler to set real time application telemetry timers
///     
///</summary>
void setRealTimeTelemetryInterval(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Updte the variable referenced in the twin table
    *(int *)(twin_t*)localTwinPtr->twinVar = (int)json_object_get_number(desiredProperties, localTwinPtr->twinKey);
    Log_Debug("Received device update. New %s is %d\n", localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
    
    // If the passed in integer value is negitive, set the variable to 0 (telemetry off)
    if(*(int *)localTwinPtr->twinVar < 0){
        *(int *)localTwinPtr->twinVar = 0;
    }

    // Send the new interval to the real time application(s), don't overflow the 16bit variable
    sendRealTimeTelemetryInterval(IC_SET_SAMPLE_RATE, (*(int *)localTwinPtr->twinVar > UINT32_MAX)? UINT32_MAX : (uint32_t)(*(int *)localTwinPtr->twinVar));

    // Send the reported property to the IoTHub
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
}
#endif 

/// <summary>
///     Send a variable number of {"key": value} json pairs
///
///     Example CallS:
///             updateDeviceTwin(false, ARGS_PER_TWIN_ITEM*4, 
///                                     TYPE_STRING, "model", "My Model Name",
///                                     TYPE_BOOL, "boolKey", "true",
///                                     TYPE_FLOAT, "floatKey", 123.45,
///                                     TYPE_INT, "intKey", 678 );
///
///             updateDeviceTwin(false, ARGS_PER_TWIN_ITEM*1, TYPE_STRING, "model", "My Model Name")
///
///    bool ioTRwFormat true == Response to device twin desired property change
///                     false == Read only device twin update
///
/// </summary>
Cloud_Result updateDeviceTwin(bool ioTRwFormat, int arg_count, ...)
{
      
    // Make sure we have an even number of "Key": "Value" arguments
    if(arg_count%3 == 1){
        return Cloud_Result_OtherFailure;
    }

#ifdef USE_PNP

#define LOCAL_BUFFER_SIZE 128

    // Define a canned success response to send up
	char* resultTxt = "Property successfully updated";

    // Allocate a buffer to build dynamic keys
    char* pjsonBuffer = (char *)malloc(LOCAL_BUFFER_SIZE);
    if (pjsonBuffer == NULL) {
		Log_Debug("ERROR: not enough memory to report device twin changes.");
	}

#endif // USE_PNP

    // Prepare the argument list
    va_list inputList;
    va_start(inputList, arg_count);

    // Send static device twin values
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serializedJson = NULL;
    
    // Consume the data in the argument list and build out the json
    for(int i = 0; i < arg_count/3; i++){
#ifdef USE_PNP
        if(ioTRwFormat){

            // Pull the data type from the list 
            int dataType = va_arg(inputList, int);

            // Pull the current "key"
            char* keyString = va_arg(inputList, char*);

            // "<newKey>?.value: "
            snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "%s.value", keyString);	
            switch (dataType) {

	    	    // report current device twin data as reported properties to IoTHub
		        case TYPE_BOOL:
                    json_object_dotset_boolean(root_object, pjsonBuffer, va_arg(inputList, int)? 1: 0);
			        break;
		        case TYPE_FLOAT:
                    json_object_dotset_number(root_object, pjsonBuffer, va_arg(inputList, double));
			        break;
		        case TYPE_INT:
                    json_object_dotset_number(root_object, pjsonBuffer, va_arg(inputList, int));
			        break;
 		        case TYPE_STRING:
                    json_object_dotset_string(root_object, pjsonBuffer, va_arg(inputList, char*));
			        break;
	        }

            snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "%s.ac", keyString);	
            json_object_dotset_number(root_object, pjsonBuffer, 200);
    
            snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "%s.av", keyString);	
            json_object_dotset_number(root_object, pjsonBuffer, desiredVersion);

            snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "%s.ad", keyString);	
            json_object_dotset_string(root_object, pjsonBuffer, resultTxt);

        }
        else // Not PnP Formatted
#endif // #ifdef USE_PNP        
        { 

            int dataType = va_arg(inputList, int);
            // Pull the data type from the list 
            switch (dataType) {

		        // report current device twin data as reported properties to IoTHub
		        case TYPE_BOOL:
                    json_object_dotset_boolean(root_object, va_arg(inputList, char*), va_arg(inputList, int)? 1: 0);
			        break;
		        case TYPE_FLOAT:
                    json_object_dotset_number(root_object, va_arg(inputList, char*), va_arg(inputList, double));
			        break;
		        case TYPE_INT:
                    json_object_dotset_number(root_object, va_arg(inputList, char*), va_arg(inputList, int));
			        break;
 		        case TYPE_STRING:
                    json_object_dotset_string(root_object, va_arg(inputList, char*), va_arg(inputList, char*));
			        break;
	        }
        }
    }

    // Clean up the argument list    
    va_end(inputList);

    // Prepare the JSON to send and send it.
    serializedJson = json_serialize_to_string(root_value);
    AzureIoT_Result aziotResult = AzureIoT_DeviceTwinReportState(serializedJson, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    // Check the return value
    if (result != Cloud_Result_OK) {
        Log_Debug("WARNING: Could not send device twin update to cloud: %s\n", CloudResultToString(result));

        // Output the device update to help debug the issue
        serializedJson = json_serialize_to_string_pretty(root_value);
        Log_Debug("%s\n", serializedJson);
    }

    // Clean up
    json_free_serialized_string(serializedJson);
    json_value_free(root_value);

    return result;
}

/// <summary>
///     Callback invoked when a Device Twin update is received from Azure IoT Hub.
///     Use the device twin table to call the function to process each "key" in the message
/// </summary>
//void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
//                               size_t payloadSize, void *userContextCallback)
void DeviceTwinCallbackHandler(const char *nullTerminatedJsonString)
{

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

    // Traverse the twin table, if we find a key that's in the table, then call the function
    // defined for that key.
    for (int i = 0; i < twinArraySize; i++)
    {

        if (json_object_has_value(desiredProperties, twinArray[i].twinKey) != 0) {

            // Call the function from the table, pass in a pointer to this table entry and the
            // desired property pointer
            twinArray[i].twinHandler(&twinArray[i], desiredProperties);

        }
    }

cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
}

/// <summary>
///     Using the device twin table, send up initial device twin values as reported properties
/// </summary>
void sendInitialDeviceTwinReportedProperties(void)
{
    for (int i = 0; i < (sizeof(twinArray) / sizeof(twin_t)); i++) {

        switch (twinArray[i].twinType) {
        case TYPE_BOOL:
            Log_Debug("Send twin update. New %s is %s\n", twinArray[i].twinKey,
                      *(bool *)twinArray[i].twinVar ? "true" : "false");
            updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_BOOL, twinArray[i].twinKey, *(bool *)twinArray[i].twinVar);
            break;
        case TYPE_FLOAT:
            Log_Debug("Received device update. New %s is %0.2f\n", twinArray[i].twinKey,
                      *(float *)twinArray[i].twinVar);
            updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_FLOAT, twinArray[i].twinKey, *(float *)twinArray[i].twinVar);
            break;
        case TYPE_INT:
            Log_Debug("Received device update. New %s is %d\n", twinArray[i].twinKey,
                      *(int *)twinArray[i].twinVar);
            updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, twinArray[i].twinKey, *(int *)twinArray[i].twinVar);
            break;

        case TYPE_STRING:
            Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey,
                      (char *)twinArray[i].twinVar);
            updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_STRING, twinArray[i].twinKey, (char *)twinArray[i].twinVar);
            break;
        }
    }
}

/// <summary>
///     Traverse the device twin table.  If the entry operates on a GPIO FD, then open the FD and
///     set it's value acording to the table
/// </summary>
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

/// <summary>
///     Close any file descriptors that are managed from the device twin table
/// </summary>
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

///<summary>
///		Handler to update the sensor poll timer
///     
///</summary>
void setTelemetryTimerFunction(void* thisTwinPtr, JSON_Object *desiredProperties){

    // Declare a local variable to point to the deviceTwin table entry and cast the incomming void* to a twin_t*
    twin_t *localTwinPtr = (twin_t*)thisTwinPtr;

    // Updte the variable referenced in the twin table
    *(int *)(twin_t*)localTwinPtr->twinVar = (int)json_object_get_number(desiredProperties, localTwinPtr->twinKey);
    
    // Make sure that the new timer variable is not zero or negitive
    if(*(int *)localTwinPtr->twinVar > 0){

 	    // Define a new timespec variable for the timer and change the timer period
	    struct timespec newPeriod = { .tv_sec = *(int *)localTwinPtr->twinVar,.tv_nsec = 0 };
        SetEventLoopTimerPeriod(telemetrytxIntervalr, &newPeriod);
    }
    // If the new time is zero, then we disable the functionality.
    else if(*(int *)localTwinPtr->twinVar == 0){

        DisarmEventLoopTimer(telemetrytxIntervalr);
    }

    // Send the reported property to the IoTHub
    Log_Debug("Received device update. New %s is %d\n", localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
    updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, localTwinPtr->twinKey, *(int *)localTwinPtr->twinVar);
}
#endif // IOT_HUB_APPLICATION