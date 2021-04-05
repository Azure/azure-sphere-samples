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

/*
Direct Method implementation for Azure Sphere


    // The dmInitFunction if defined will be called at powerup from the dmInit() routine
    typedef sig_atomic_t (*dmInitFunction)(void*);

    // The dmHandler takes the payload to process and returns a an HTTP result and a pointer to a response message on the heap
    typedef int (*dmHandler)(JSON_Object *JsonPayloadObj, size_t payloadSize, char* responsePayload);

    // The dmCleanup handler is called at system exit to cleanup/release any system resources
    typedef void (*dmCleanup)(void);


*/

#include "direct_methods.h"
#include "exit_codes.h"

#ifdef IOT_HUB_APPLICATION

#include <stdlib.h>

// Define each direct methodthat we plan to process
// .dmName - The direct method name
// .dmPayloadRequired - Does the direct method require a payload?
// .dmInit - Init function called at power up, NULL if not required
// .dmHandler: The handler that will be called for this direct method.  The function must have the same signaure 
// void <yourFunctionName>(void* thisTwinPtr, JSON_Object *desiredProperties);
// .dmCleanup - The handler that will be called at application exit time, NULL if not required
direct_method_t dmArray[] = {
	{.dmName = "test",.dmPayloadRequired=true,.dmInit=dmTestInitFunction,.dmHandler=dmTestHandlerFunction,.dmCleanup=dmTestCleanupFunction},
    {.dmName = "rebootDevice",.dmPayloadRequired=false,.dmInit=dmRebootInitFunction,.dmHandler=dmRebootHandlerFunction,.dmCleanup=dmRebootCleanupFunction},
	{.dmName = "setSensorPollTime",.dmPayloadRequired=true,.dmInit=NULL,.dmHandler = dmSetTelemetryTxTimeHandlerFunction,.dmCleanup=NULL},
    // Reproduce the rebootDevice entry but use a different direct method name.  This change maintains compatability with the 
    // IoTCentral application we share with the community.  Set the init and cleanup pointers to NULL since we'll use the same timer
    // as the rebootDevice direct method.
    {.dmName = "haltApplication",.dmPayloadRequired=false,.dmInit=NULL,.dmHandler=dmRebootHandlerFunction,.dmCleanup=NULL}
};

// Calculate how many twin_t items are in the array.  We use this to iterate through the structure.
int dmArraySize = sizeof(dmArray)/sizeof(direct_method_t);

/// <summary>
///     InitDirectMethods(void)
///     Traverse the direct method table and call the init routine if defined
/// </summary>
sig_atomic_t InitDirectMethods(void){

ExitCode result = ExitCode_Success;

    // Traverse the DM table, call the init routine if defined
    for (int i = 0; i < dmArraySize; i++)
    {
        // If this entry has an init routine call it
        if (dmArray[i].dmInit != NULL) {
            result = dmArray[i].dmInit(&dmArray[i]);
            if(result != ExitCode_Success){
                return result;
            }
        }
    }

    return result;
}

void CleanupDirectMethods(void){

    // Traverse the DM table, call the cleanup routine if defined
    for (int i = 0; i < dmArraySize; i++)
    {
        // If this entry has an init routine call it
        if (dmArray[i].dmInit != NULL) {
            dmArray[i].dmCleanup();
        }
    }
}

/// <summary>
///     Direct Method callback function, called when a Direct Method call is received from the Azure
///     IoT Hub.
/// </summary>
/// <param name="methodName">The name of the method being called.</param>
/// <param name="payload">The payload of the method.</param>
/// <param name="responsePayload">The response payload content. This must be a heap-allocated
/// string, 'free' will be called on this buffer by the Azure IoT Hub SDK.</param>
/// <param name="responsePayloadSize">The size of the response payload content.</param>
/// <returns>200 HTTP status code if the method name is reconginized and the payload is correctly parsed;
/// 400 HTTP status code if the payload is invalid;</returns>
/// 404 HTTP status code if the method name is unknown.</returns>
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, size_t payloadSize, 
                         unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback)
{

    size_t mallocSize = 0;
    static const char errorResponseNoMethod[] = "{\"success\": false, \"message\" : \"Direct Method %s not found\"}";
    static const char errorResponseBadPayload[] = "{\"success\": false, \"message\" : \"Invalid payload for Direct Method %s\"}";
    static const char successResponse[] = "{\"success\": true}";
    char* cannedResponse = (char*)&successResponse;

    // HTTP status code method name is unknown
    int result = 404;
    
    // Pointer to a response message.  Each direct method implementation will 
    // create its specific response on the heap and return the pointer.  This
    // routine will pass that pointer back to the system where the message will
    // be sent to the IoTHub and then the memory will be freed
    char* responseMsg = NULL;

    // Pointer to a copy of the passed in payload.  We'll malloc memory for this
    // data and null terminate it so that the parson library can help us process
    // the JSON message.
    char* directMethodPayload;

    // Pointers to the parsable JSON payload
    JSON_Value* payloadJson = NULL;
    JSON_Object* payloadJsonObj = NULL;

    Log_Debug("Received Device Method callback: Method name %s.\n", methodName);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Step1: Prepare the JSON payload for processing
    //
    /////////////////////////////////////////////////////////////////////////////

    // Copy the payload on to the heap then null terminate it
    // The maximum size direct method payload is 128KB
    directMethodPayload = malloc(payloadSize+1);
	memcpy(directMethodPayload, payload, payloadSize);
	directMethodPayload[payloadSize] = '\0';
	
    // Note this call allocates memory and must be freed by calling json_value_free(payloadJson)
    payloadJson = json_parse_string(directMethodPayload);


    // Verify we have a valid JSON string from the payload
	if (payloadJson == NULL) {
        result = 400;
        goto payloadError;
	}

    /////////////////////////////////////////////////////////////////////////////
    //
    // Step2: Find the direct method in the dm array and call the handler
    //
    /////////////////////////////////////////////////////////////////////////////


    // Traverse the DM array looking for a matching entry for the method called.  We assume that there is
    // only one matching entry in the table for this method and exit the loop after finding/calling the methods
    // handler function.
    for (int i = 0; i < dmArraySize; i++)
    {
        if (strcmp(methodName, dmArray[i].dmName) == 0) {

            payloadJsonObj = json_value_get_object(payloadJson);

        	// Check to see if this entry requires a payload, if not skip the get_object call
            if ((dmArray[i].dmPayloadRequired) && (payloadJsonObj == NULL)){
                cannedResponse = (char*)&errorResponseBadPayload;
                goto payloadError;

            }

            // Call the handler for this entry
            result = dmArray[i].dmHandler(payloadJsonObj, payloadSize, &responseMsg);
            break;
        }
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    // Step3: Make sure there is a response to return to the Azure IoT library
    //
    /////////////////////////////////////////////////////////////////////////////

    // Look at the result code to determine how to proceed
    switch(result){

    // 200 HTTP status code if the method name is reconginized and the payload is correctly parsed;
    case 200:
        // Check to see if the direct method handler created a response message on the heap, if so 
        // then update the response payload variables to return to the Azure IoT library
        if(responseMsg != NULL){

            // Update the "pass value by reference" variables
            *responsePayloadSize = strlen(responseMsg);
            *responsePayload = responseMsg;
            goto cleanup;
        }
        // The direct method did not create a response message, but it returned 200, so create a canned
        // success message to return to the Azure IoT library
        else{

		    // Construct the response message.  This response will be displayed in the cloud when calling the direct method
            // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
		    mallocSize = strlen(successResponse);
            *responsePayload = malloc(mallocSize);
            if (*responsePayload == NULL) {

			    Log_Debug("ERROR: SetupHeapMessage error.\n");
		        exitCode = ExitCode_DirectMethodResponse_Malloc_failed;
                abort();
		    }
        
            // Copy the canned success response string into the dynamic memory
            strncpy (*responsePayload, successResponse, strlen(successResponse));
            *responsePayloadSize = mallocSize;
            goto cleanup;
        }
        break;
    
    // If the direct method handler returned one of the error cases, then generate a canned error
    // response.
    case 404: // 404 HTTP status code if the method name is unknown
        cannedResponse = (char*)&errorResponseNoMethod;
        goto payloadError;
    case 400: // 400 HTTP status code if the payload is invalid
        cannedResponse = (char*)&errorResponseBadPayload;
    default:
        goto payloadError;
        break;
    }
    
payloadError:

    // Set the result to the payload error code
    result = 400;

	// Construct the response message.  This response will be displayed in the cloud when calling the direct method
    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
	mallocSize = strlen(cannedResponse) + strlen(methodName);
    *responsePayload = malloc(mallocSize);
    if (*responsePayload == NULL) {

		Log_Debug("ERROR: SetupHeapMessage error.\n");
		exitCode = ExitCode_DirectMethodError_Malloc_failed;    
        abort();
    }
        
    // Construct the response message
    *responsePayloadSize = (size_t)snprintf(*responsePayload, mallocSize, cannedResponse, methodName);

cleanup:    
    // Release the memory allocated by this routine
    // Note memory must be released in this order since payloadJson refers to directMethodPayload
    json_value_free(payloadJson);
   	free (directMethodPayload);
    
    return result;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for test example directMethod
//
//  name: test
//  Payload: {}, or {"returnVal": <200|400|404>}
//
//////////////////////////////////////////////////////////////////////////////////////


// The dmInitFunction if defined will be called at powerup from the dmInit() routine
sig_atomic_t dmTestInitFunction(void* thisDmEntry){

    direct_method_t* thisDmArrayEntry = (direct_method_t*)thisDmEntry;
    Log_Debug("%s DirectMethod initFunction Called\n", thisDmArrayEntry->dmName);
    return ExitCode_Success;
}

// The dmHandler takes the payload to process and returns a pointer to a response message on the heap
int dmTestHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responseMsg){

    // Assume that the routine will return success
    int result = 200;

	// Verify that the payloadJson contains a valid JSON object
	if (JsonPayloadObj != NULL) {

        // Pull the Key: value pair from the JSON object, we're looking for {"returnVal": <integer>}
    	result  = (int)json_object_get_number(JsonPayloadObj, "returnVal");
        if (result == 0) {
		
            // There was no payload, return success for this test routine
            result = 200;
	    }

	}
    else{
        // There was not a valid payload, just return success for this test method
        // result was set to 200 above
    }

    return result;
    
}

// The dmCleanup handler is called at system exit to cleanup/release any system resources
void dmTestCleanupFunction(void){

    Log_Debug("DirectMethod cleanup called\n");

}

//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for setSensortxInterval directMethod
//
//  name: setSensortxInterval
//  payload: {"pollTime": <integer >0 >}
//
//////////////////////////////////////////////////////////////////////////////////////

int dmSetTelemetryTxTimeHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responseMsg){

	// Pull the Key: value pair from the JSON object, we're looking for {"pollTime": <integer>}
	// Verify that the new timer is > 1
	int pollTime = (int)json_object_get_number(JsonPayloadObj, "pollTime");
	
    if (pollTime < 1) {
		return 400;
	}

	// Construct the response message.  This will be displayed in the cloud when calling the direct method
	static const char newtxIntervalResponse[] = "{ \"success\" : true, \"message\" : \"New telemetry tx interval %d seconds\" }";
    size_t mallocSize = sizeof(newtxIntervalResponse) + 8;  // Add 8 to cover the txInterval integer that will be inserted into the response string
	*responseMsg = (char *)malloc(mallocSize); 

    if (*responseMsg == NULL) {
	    exitCode = ExitCode_SettxInterval_Malloc_failed;
		return 400;
	}
  
    // Construct the response message
    snprintf(*responseMsg, mallocSize, newtxIntervalResponse, pollTime);

    // Make sure that the new timer variable is not zero or negitive
    if(pollTime > 0){

    	// Define a new timespec variable for the timer and change the timer period
	    struct timespec newAccelReadPeriod = { .tv_sec = pollTime,.tv_nsec = 0 };
        SetEventLoopTimerPeriod(sensorPollTimer, &newAccelReadPeriod);

    }
    // If the new time is zero, then we disable the functionality.
    else if(pollTime == 0){

        DisarmEventLoopTimer(sensorPollTimer);
    }
	return 200;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for rebootDevice example directMethod
//
//  name: rebootDevice
//  Payload: {"delayTime": <delay in seconds >0 >}
//
//////////////////////////////////////////////////////////////////////////////////////

// The dmHandler takes the payload to process and returns a pointer to a response message on the heap
int dmRebootHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responseMsg){

	// Pull the Key: value pair from the JSON object, we're looking for {"delayTime": <integer>}
	// Verify that the new timer is > 1
	int delayTime = (int)json_object_get_number(JsonPayloadObj, "delayTime");
	
    if (delayTime < 1) {
		return 400;
	}

	// Construct the response message.  This will be displayed in the cloud when calling the direct method
	static const char rebootResponse[] = "{ \"success\" : true, \"message\" : \"Rebooting Device in %d seconds\"}";
    size_t mallocSize = sizeof(rebootResponse) + 8;  // Add 8 to cover the reboot time that will be inserted into the response string
	*responseMsg = (char *)malloc(mallocSize); 

    if (*responseMsg == NULL) {
	    exitCode = ExitCode_SettxInterval_Malloc_failed;
		return 400;
	}
  
    // Construct the response message
    snprintf(*responseMsg, mallocSize, rebootResponse, delayTime);

    // Declare a timer and handler for the rebootDevice Direct Method
    // When the timer expires, the application will exit
    struct timespec rebootDeviceTimerTime = { .tv_sec = delayTime,.tv_nsec = 0 };
    SetEventLoopTimerOneShot(rebootDeviceTimer, &rebootDeviceTimerTime);

	return 200;
    
}

/// <summary>
///     halt application timer event:  Exit the application
/// </summary>
static void RebootDeviceEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AzureTimer_Consume;
        return;
    }

    // Force the reboot!
    PowerManagement_ForceSystemReboot();

}

// The dmInitFunction if defined will be called at powerup from the dmInit() routine
sig_atomic_t dmRebootInitFunction(void* thisDmEntry){

    // Setup the halt application handler and timer.  This is disarmed and will only fire
    // if we receive a halt application direct method call
    rebootDeviceTimer = CreateEventLoopDisarmedTimer(eventLoop, RebootDeviceEventHandler);
    
    if (rebootDeviceTimer == NULL) {
        return ExitCode_Init_RebootTimer;    
    }
    
    return ExitCode_Success;
}

// The dmCleanup handler is called at system exit to cleanup/release any system resources
void dmRebootCleanupFunction(void){

    DisposeEventLoopTimer(rebootDeviceTimer);
}
#endif 