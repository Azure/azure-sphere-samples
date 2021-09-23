/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "parson.h"

#include "azure_iot.h"
#include "cloud.h"
#include "exitcodes.h"

// This file implements the interface described in cloud.h in terms of an Azure IoT Hub.
// Specifically, it translates Azure IoT Hub specific concepts (events, device twin messages, device
// methods, etc) into business domain concepts (telemetry, upload enabled, alarm raised)

static const char azureSphereModelId[] = "dtmi:com:example:azuresphere:thermometer;1";

// Azure IoT Hub callback handlers
static void DeviceTwinCallbackHandler(const char *nullTerminatedJsonString);
static int DeviceMethodCallbackHandler(const char *methodName, const unsigned char *payload,
                                       size_t payloadSize, unsigned char **response,
                                       size_t *responseSize);
static void ConnectionChangedCallbackHandler(bool connected);

// Default handlers for cloud events
static void DefaultTelemetryUploadEnabledChangedHandler(bool uploadEnabled);
static void DefaultDisplayAlertHandler(const char *alertMessage);
static void DefaultConnectionChangedHandler(bool connected);

// Cloud event callback handlers
static Cloud_TelemetryUploadEnabledChangedCallbackType
    thermometerTelemetryUploadEnabledChangedCallbackFunction =
        DefaultTelemetryUploadEnabledChangedHandler;
static Cloud_DisplayAlertCallbackType displayAlertCallbackFunction = DefaultDisplayAlertHandler;
static Cloud_ConnectionChangedCallbackType connectionChangedCallbackFunction =
    DefaultConnectionChangedHandler;

// Utility functions
static Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result);
static bool BuildUtcDateTimeString(char *outputBuffer, size_t outputBufferSize, time_t t);

// Constants
#define MAX_PAYLOAD_SIZE 512
#define DATETIME_BUFFER_SIZE 128

// State
static unsigned int latestVersion = 1;
static char dateTimeBuffer[DATETIME_BUFFER_SIZE];

ExitCode Cloud_Initialize(EventLoop *el, void *backendContext,
                          ExitCode_CallbackType failureCallback,
                          Cloud_TelemetryUploadEnabledChangedCallbackType
                              thermometerTelemetryUploadEnabledChangedCallback,
                          Cloud_DisplayAlertCallbackType displayAlertCallback,
                          Cloud_ConnectionChangedCallbackType connectionChangedCallback)
{
    if (thermometerTelemetryUploadEnabledChangedCallback != NULL) {
        thermometerTelemetryUploadEnabledChangedCallbackFunction =
            thermometerTelemetryUploadEnabledChangedCallback;
    }

    if (displayAlertCallback != NULL) {
        displayAlertCallbackFunction = displayAlertCallback;
    }

    if (connectionChangedCallback != NULL) {
        connectionChangedCallbackFunction = connectionChangedCallback;
    }

    AzureIoT_Callbacks callbacks = {
        .connectionStatusCallbackFunction = ConnectionChangedCallbackHandler,
        .deviceTwinReceivedCallbackFunction = DeviceTwinCallbackHandler,
        .deviceTwinReportStateAckCallbackTypeFunction = NULL,
        .sendTelemetryCallbackFunction = NULL,
        .deviceMethodCallbackFunction = DeviceMethodCallbackHandler};

    return AzureIoT_Initialize(el, failureCallback, azureSphereModelId, backendContext, callbacks);
}

void Cloud_Cleanup(void)
{
    AzureIoT_Cleanup();
}

static Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result)
{
    switch (result) {
    case AzureIoT_Result_OK:
        return Cloud_Result_OK;
    case AzureIoT_Result_NoNetwork:
        return Cloud_Result_NoNetwork;
    case AzureIoT_Result_OtherFailure:
    default:
        return Cloud_Result_OtherFailure;
    }
}

Cloud_Result Cloud_SendTelemetry(const Cloud_Telemetry *telemetry, time_t timestamp)
{
    char *utcDateTime = NULL;
    if (timestamp != -1) {
        BuildUtcDateTimeString(dateTimeBuffer, sizeof(dateTimeBuffer), timestamp);
        utcDateTime = dateTimeBuffer;
    }

    JSON_Value *telemetryValue = json_value_init_object();
    JSON_Object *telemetryRoot = json_value_get_object(telemetryValue);
    json_object_dotset_number(telemetryRoot, "temperature", telemetry->temperature);
    char *serializedTelemetry = json_serialize_to_string(telemetryValue);
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(serializedTelemetry, utcDateTime, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedTelemetry);
    json_value_free(telemetryValue);

    return result;
}

Cloud_Result Cloud_SendThermometerMovedEvent(time_t timestamp)
{
    char *utcDateTime = NULL;
    if (timestamp != -1) {
        BuildUtcDateTimeString(dateTimeBuffer, sizeof(dateTimeBuffer), timestamp);
        utcDateTime = dateTimeBuffer;
    }

    JSON_Value *thermometerMovedValue = json_value_init_object();
    JSON_Object *thermometerMovedRoot = json_value_get_object(thermometerMovedValue);
    json_object_dotset_boolean(thermometerMovedRoot, "thermometerMoved", 1);
    char *serializedDeviceMoved = json_serialize_to_string(thermometerMovedValue);
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(serializedDeviceMoved, utcDateTime, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedDeviceMoved);
    json_value_free(thermometerMovedValue);

    return result;
}

Cloud_Result Cloud_SendThermometerTelemetryUploadEnabledChangedEvent(bool uploadEnabled)
{
    JSON_Value *thermometerTelemetryUploadValue = json_value_init_object();
    JSON_Object *thermometerTelemetryUploadRoot =
        json_value_get_object(thermometerTelemetryUploadValue);

    json_object_dotset_boolean(thermometerTelemetryUploadRoot,
                               "thermometerTelemetryUploadEnabled.value", uploadEnabled ? 1 : 0);
    json_object_dotset_number(thermometerTelemetryUploadRoot,
                              "thermometerTelemetryUploadEnabled.ac", 200);
    json_object_dotset_number(thermometerTelemetryUploadRoot,
                              "thermometerTelemetryUploadEnabled.av", latestVersion++);
    json_object_dotset_string(thermometerTelemetryUploadRoot,
                              "thermometerTelemetryUploadEnabled.ad",
                              "Successfully updated thermometerTelemetryUploadEnabled");
    char *serializedTelemetryUpload = json_serialize_to_string(thermometerTelemetryUploadValue);
    AzureIoT_Result aziotResult = AzureIoT_DeviceTwinReportState(serializedTelemetryUpload, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedTelemetryUpload);
    json_value_free(thermometerTelemetryUploadValue);

    return result;
}

Cloud_Result Cloud_SendDeviceDetails(const char *serialNumber)
{
    // Send static device twin properties when connection is established.
    JSON_Value *deviceDetailsValue = json_value_init_object();
    JSON_Object *deviceDetailsRoot = json_value_get_object(deviceDetailsValue);
    json_object_dotset_string(deviceDetailsRoot, "serialNumber", serialNumber);
    char *serializedDeviceDetails = json_serialize_to_string(deviceDetailsValue);
    AzureIoT_Result aziotResult = AzureIoT_DeviceTwinReportState(serializedDeviceDetails, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedDeviceDetails);
    json_value_free(deviceDetailsValue);

    return result;
}

static bool BuildUtcDateTimeString(char *outputBuffer, size_t outputBufferSize, time_t t)
{
    // Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
    static const char *ISO8601Format = "%Y-%m-%dT%H:%M:%SZ";

    bool result;

    struct tm *currentTimeTm;
    currentTimeTm = gmtime(&t);

    if (strftime(outputBuffer, outputBufferSize, ISO8601Format, currentTimeTm) == 0) {
        Log_Debug("ERROR: strftime: %s (%d)\n", errno, strerror(errno));
        result = false;
    } else {
        result = true;
    }

    return result;
}

static void DefaultTelemetryUploadEnabledChangedHandler(bool uploadEnabled)
{
    Log_Debug("WARNING: Cloud - no handler registered for TelemetryUploadEnabled - status %s\n",
              uploadEnabled ? "true" : "false");
}

static void DefaultDisplayAlertHandler(const char *alertMessage)
{
    Log_Debug("WARNING: Cloud - no handler registered for DisplayAlert - message %s\n",
              alertMessage);
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

static void DeviceTwinCallbackHandler(const char *nullTerminatedJsonString)
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

    // The desired properties should have a "TelemetryUploadEnabled" object
    int thermometerTelemetryUploadEnabledValue =
        json_object_dotget_boolean(desiredProperties, "thermometerTelemetryUploadEnabled");

    if (thermometerTelemetryUploadEnabledValue != -1) {
        unsigned int requestedVersion =
            (unsigned int)json_object_dotget_number(desiredProperties, "$version");

        if (requestedVersion > latestVersion) {
            latestVersion = requestedVersion;
        }

        thermometerTelemetryUploadEnabledChangedCallbackFunction(
            thermometerTelemetryUploadEnabledValue == 1);
    }

cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
}

static int DeviceMethodCallbackHandler(const char *methodName, const unsigned char *payload,
                                       size_t payloadSize, unsigned char **response,
                                       size_t *responseSize)
{
    int result;
    char *responseString;
    static char nullTerminatedPayload[MAX_PAYLOAD_SIZE + 1];

    size_t actualPayloadSize = payloadSize > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : payloadSize;

    strncpy(nullTerminatedPayload, payload, actualPayloadSize);
    nullTerminatedPayload[actualPayloadSize] = '\0';

    if (strcmp("displayAlert", methodName) == 0) {

        displayAlertCallbackFunction(nullTerminatedPayload);

        responseString =
            "\"Alert message displayed successfully.\""; // must be a JSON string (in quotes)
        result = 200;
    } else {
        // All other method names are ignored
        responseString = "{}";
        result = -1;
    }

    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
    *responseSize = strlen(responseString);
    *response = malloc(*responseSize);
    memcpy(*response, responseString, *responseSize);

    return result;
}
