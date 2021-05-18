/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <memory.h>
#include <stdlib.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "parson.h"

#include "azure_iot.h"
#include "cloud.h"
#include "exitcodes.h"

// Avnet additions
#include "build_options.h"
#include "../avnet/device_twin.h"
#include "../avnet/direct_methods.h"
#include "../avnet/m4_support.h"


// This file implements the interface described in cloud.h in terms of an Azure IoT Hub.
// Specifically, it translates Azure IoT Hub specific concepts (events, device twin messages, device
// methods, etc) into business domain concepts (telemetry, upload enabled, alarm raised)

// The model ID constant can be modified in the build_options.h file
static const char azureSpherePnPModelId[] = IOT_PLUG_AND_PLAY_MODEL_ID;

// Timer used to sending periodic telemetry data
EventLoopTimer *telemetrytxIntervalr = NULL;

// Azure IoT Hub callback handlers
static void ConnectionChangedCallbackHandler(bool connected);

// Default handlers for cloud events
static void DefaultConnectionChangedHandler(bool connected);

// Cloud event callback handlers
static Cloud_ConnectionChangedCallbackType connectionChangedCallbackFunction =
    DefaultConnectionChangedHandler;

//unsigned int latestVersion = 1;

ExitCode Cloud_Initialize(EventLoop *el, void *backendContext,
                          ExitCode_CallbackType failureCallback,
                          Cloud_DisplayAlertCallbackType displayAlertCallback,
                          Cloud_ConnectionChangedCallbackType connectionChangedCallback)
{
    if (connectionChangedCallback != NULL) {
        connectionChangedCallbackFunction = connectionChangedCallback;
    }

    // Set up a timer to send telemetry.  SEND_TELEMETRY_PERIOD_SECONDS is defined in build_options.h
    // This timer can be modified from the cloud using either a direct method or a devcie twin
    static const struct timespec sendTelemetryPeriod = {.tv_sec = SEND_TELEMETRY_PERIOD_SECONDS,
                                                     .tv_nsec = SEND_TELEMETRY_PERIOD_NANO_SECONDS};
    telemetrytxIntervalr = CreateEventLoopPeriodicTimer(eventLoop, &SendTelemetryTimerEventHandler, &sendTelemetryPeriod);
    if (telemetrytxIntervalr == NULL) {
        return ExitCode_Init_TelemetrytxIntervalr;
    }

    AzureIoT_Callbacks callbacks = {
        .connectionStatusCallbackFunction = ConnectionChangedCallbackHandler,
        .deviceTwinReceivedCallbackFunction = DeviceTwinCallbackHandler,
        .deviceTwinReportStateAckCallbackTypeFunction = NULL,
        .sendTelemetryCallbackFunction = NULL,
        .deviceMethodCallbackFunction = DeviceMethodCallbackHandler};

    return AzureIoT_Initialize(el, failureCallback, azureSpherePnPModelId, backendContext, callbacks);
}

void Cloud_Cleanup(void)
{
    AzureIoT_Cleanup();

#ifdef IOT_HUB_APPLICATION    
#ifdef M4_INTERCORE_COMMS    
    CleanupM4Resources();
#endif // M4_INTERCORE_COMMS    
    deviceTwinCloseFDs();
#endif // IOT_HUB_APPLICATION    
}

Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result)
{
    switch (result) {
    case AzureIoT_Result_OK:
        return Cloud_Result_OK;
    case AzureIoT_Result_NoNetwork:
        return Cloud_Result_NoNetwork;
    case AzureIoT_Result_NotAuthenticated:
        return Cloud_Result_NotAuthenticated;
    case AzureIoT_Result_SendReportedState_Failed:
        return Cloud_SendFailed;   
    case AzureIoT_Result_OtherFailure:
    default:
        return Cloud_Result_OtherFailure;
    }
}

/// <summary>
///     telemetrytxIntervalr timer event:  Send telemetry
///
///     Note, this handler does not read any sensors, but it should send any
///     current sensor data up in a telemetry message.
/// </summary>
void SendTelemetryTimerEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }

#ifdef M4_INTERCORE_COMMS
    // Send each real time core a message requesting telemetry
    RequestRealTimeTelemetry();

#endif     

    // Send an example telemetry message
    Cloud_SendTelemetry(true, 3*ARGS_PER_TELEMETRY_ITEM, 
                              TYPE_STRING, "sampleKeyString", "AvnetKnowsIoT",
                              TYPE_INT, "sampleKeyInt", (int)(rand()%100),
                              TYPE_FLOAT, "sampleKeyFloat", ((float)rand()/(float)(RAND_MAX)) * 100);

#ifdef IOT_HUB_APPLICATION
//    SendTelemetry(pjsonBuffer, true);
#else
    Log_Debug("Not sending telemetry, non-IoT Hub build\n");
#endif // IOT_HUB_APPLICATION
}

/// <summary>
///     Send a variable number of "key": value pairs
///
///     Example CallS:
///             Cloud_SendTelemetry(false, 12, 
///                                     TYPE_STRING, "model", "My Model Name",
///                                     TYPE_BOOL, "boolKey", "true",
///                                     TYPE_FLOAT, "floatKey", 123.45,
///                                     TYPE_INT, "intKey", 678 );
///
///             Cloud_SendTelemetry(false, 3, TYPE_STRING, "model", "My Model Name")
///
/// </summary>
Cloud_Result Cloud_SendTelemetry(bool IoTConnectFormat, int arg_count, ...)
{
    Cloud_Result result = 1;
    char *serializedJson = NULL;

    // Make sure we have an triple number of {DataType, "Key", "Value"} arguments
    if(arg_count%3 == 1){
        return Cloud_Result_OtherFailure;
    }

    // Prepare the JSON object for the telemetry data
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

#ifdef USE_IOT_CONNECT

    //creating a Json_Array
    JSON_Value *myArrayValue = json_value_init_array();
    JSON_Array *myArray = json_value_get_array(myArrayValue);

    // Prepare the JSON object for the telemetry data
    JSON_Value *array_value_object = json_value_init_object();
    JSON_Object *array_object = json_value_get_object(array_value_object);

#define LOCAL_BUFFER_SIZE 128

    char* pjsonBuffer = (char *)malloc(LOCAL_BUFFER_SIZE);
    if (pjsonBuffer == NULL) {
		Log_Debug("ERROR: not enough memory to send telemetry.");
	}

    // If we need to format the message for IoT Connect, then do so.  We need to format the data as shown below
    // "{\"sid\":\"%s\",\"dtg\":\"%s\",\"mt\": 0,\"dt\": \"%s\",\"d\":[{\"d\":<new telemetry "key": value pairs>}]}";
    if(IoTConnectFormat){
    
        serializedJson = NULL;

        json_object_dotset_string(root_object, "sid", sidString);
        json_object_dotset_string(root_object, "dtg", dtgGUID);
        json_object_dotset_number(root_object, "mt", 0);

    }

#endif // USE_IOT_CONNECT

    // Prepare the argument list
    va_list inputList;
    va_start(inputList, arg_count);

    // Consume the data in the argument list and build out the json
    for(int i = 0; i < arg_count/3; i++){

#ifdef USE_IOT_CONNECT
        if(IoTConnectFormat){
            
            // Pull the data type from the list 
            int dataType = va_arg(inputList, int);

            // Pull the current "key"
            char* keyString = va_arg(inputList, char*);

            // "d.<newKey>: <value>"
            snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "d.%s", keyString);	
            switch (dataType) {

	    	    // report current device twin data as reported properties to IoTHub
		        case TYPE_BOOL:
                    json_object_dotset_boolean(array_object, pjsonBuffer, va_arg(inputList, int)? 1: 0);
			        break;
		        case TYPE_FLOAT:
                    json_object_dotset_number(array_object, pjsonBuffer, va_arg(inputList, double));
			        break;
		        case TYPE_INT:
                    json_object_dotset_number(array_object, pjsonBuffer, va_arg(inputList, int));
			        break;
 		        case TYPE_STRING:
                    json_object_dotset_string(array_object, pjsonBuffer, va_arg(inputList, char*));
			        break;
	        }

        }
        else
#endif // #ifdef USE_IOT_CONNECT        
        { // Not IoT Connect Formatted

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

#ifdef USE_IOT_CONNECT
    // If we're formatting for IoT Connect, then use the previously constructed *_iotc structures and add the
    // telemetry oject we just created
    if(IoTConnectFormat){

        json_array_append_value(myArray,array_value_object);
        json_object_dotset_value(root_object,"d", myArrayValue);
    }

#endif // USE_IOT_CONNECT

    // Serialize the structure and send it as telemetry
    serializedJson = json_serialize_to_string(root_value); // leaf_value
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(serializedJson, NULL);
    result = AzureIoTToCloudResult(aziotResult);

    // Clean up
    json_free_serialized_string(serializedJson);
    json_value_free(root_value);

    if (result != Cloud_Result_OK) {
        Log_Debug("WARNING: Could not send telemetry to cloud: %s\n", CloudResultToString(result));
        
        // Output the telemetry Json structure as debuh
        serializedJson = json_serialize_to_string_pretty(root_value); // leaf_value
        Log_Debug("%s\n", serializedJson);
    }

    return result;
}


Cloud_Result Cloud_SendThermometerMovedEvent(void)
{
    JSON_Value *thermometerMovedValue = json_value_init_object();
    JSON_Object *thermometerMovedRoot = json_value_get_object(thermometerMovedValue);
    json_object_dotset_boolean(thermometerMovedRoot, "thermometerMoved", 1);
    char *serializedDeviceMoved = json_serialize_to_string(thermometerMovedValue);
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(serializedDeviceMoved, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedDeviceMoved);
    json_value_free(thermometerMovedValue);

    return result;
}

static void DefaultConnectionChangedHandler(bool connected)
{
    Log_Debug("WARNING: Cloud - no handler registered for ConnectionChanged - status %s\n",
              connected ? "true" : "false");
}

static void ConnectionChangedCallbackHandler(bool connected)
{
    connectionChangedCallbackFunction(connected);
}

const char *CloudResultToString(Cloud_Result result)
{
    switch (result) {
    case Cloud_Result_OK:
        return "OK";
    case Cloud_Result_NoNetwork:
        return "No network connection available";
    case Cloud_Result_NotAuthenticated:
        return "Device not Authenticated to IoT Hub";
    case Cloud_SendFailed:
        return "IoT Send call failed";
    case Cloud_Result_OtherFailure:
        return "Other failure";
    }

    return "Unknown Cloud_Result";
}
