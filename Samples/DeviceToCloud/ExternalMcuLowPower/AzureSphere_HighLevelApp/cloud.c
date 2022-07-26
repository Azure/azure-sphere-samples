/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This contains an implementation of the cloud.h header specialised for the Azure IoT Central
// cloud backend

#include <string.h>
#include <stddef.h>
#include <applibs/log.h>

#include "azure_iot.h"
#include "cloud.h"
#include "exitcodes.h"
#include "telemetry.h"

#include "parson.h"

#include "connection_dps.h"

static const size_t MAX_SCOPEID_LENGTH = 16;

static const int sendTelemetryMessageIdentifier = 0x01;
static const int acknowledgeFlavorMessageIdentifier = 0x02;

static bool isConnected = false;

static Connection_Dps_Config dpsConfig;

static Cloud_FlavorReceivedCallbackType flavorReceivedCallbackFunc;
static Cloud_ConnectionStatusCallbackType connectionStatusCallbackFunc;
static Cloud_SendTelemetryCallbackType sendTelemetryCallbackFunc = NULL;
static Cloud_FlavorAcknowledgementCallbackType flavorAckCallbackFunc = NULL;

static void HandleConnectionStatusChange(bool connected);
static void HandleDeviceTwinCallback(const char *content);
static void HandleDeviceTwinUpdateAckCallback(bool success, void *context);
static void HandleSendTelemetryCallback(bool success, void *context);
static void SendDeviceTwinUpdate(const char *flavorName, const char *flavorColor);

ExitCode Cloud_Initialize(EventLoop *el, void *backendConfiguration,
                          ExitCode_CallbackType failureCallback,
                          Cloud_ConnectionStatusCallbackType connectionStatusCallback,
                          Cloud_FlavorReceivedCallbackType flavorReceivedCallback)
{
    isConnected = false;
    connectionStatusCallbackFunc = connectionStatusCallback;
    flavorReceivedCallbackFunc = flavorReceivedCallback;

    AzureIoT_Callbacks cbs = {
        .connectionStatusCallbackFunction = HandleConnectionStatusChange,
        .deviceTwinReceivedCallbackFunction = HandleDeviceTwinCallback,
        .deviceTwinReportStateAckCallbackTypeFunction = HandleDeviceTwinUpdateAckCallback,
        .sendTelemetryCallbackFunction = HandleSendTelemetryCallback,
        .deviceMethodCallbackFunction = NULL};

    dpsConfig.scopeId = (char *)strndup((char *)backendConfiguration, MAX_SCOPEID_LENGTH);
    if (dpsConfig.scopeId == NULL) {
        return ExitCode_Init_CopyScopeId;
    }

    return AzureIoT_Initialize(el, failureCallback, NULL, &dpsConfig, cbs);
}

void Cloud_Cleanup(void)
{
    AzureIoT_Cleanup();
}

bool Cloud_SendTelemetry(const CloudTelemetry *telemetry,
                         Cloud_SendTelemetryCallbackType sendTelemetryCallback)
{
    if (!isConnected) {
        return false;
    }

    JSON_Value *telemetryRootValue = json_value_init_object();

    if (telemetryRootValue == NULL) {
        return false;
    }

    sendTelemetryCallbackFunc = sendTelemetryCallback;

    JSON_Object *telemetryRootObject = json_value_get_object(telemetryRootValue);

    json_object_dotset_number(telemetryRootObject, "DispensesSinceLastUpdate",
                              telemetry->dispensesSinceLastSync);
    json_object_dotset_number(telemetryRootObject, "RemainingDispenses",
                              telemetry->remainingDispenses);
    json_object_dotset_boolean(telemetryRootObject, "LowSoda", telemetry->lowSoda);
    json_object_dotset_number(telemetryRootObject, "LifetimeTotalDispenses",
                              telemetry->lifetimeTotalDispenses);
    json_object_dotset_number(telemetryRootObject, "BatteryLevel", telemetry->batteryLevel);

    char *serializedTelemetry = json_serialize_to_string(telemetryRootValue);
    AzureIoT_SendTelemetry(serializedTelemetry, NULL, (void *)&sendTelemetryMessageIdentifier);
    json_free_serialized_string(serializedTelemetry);

    json_value_free(telemetryRootValue);

    return true;
}

bool Cloud_SendFlavorAcknowledgement(const LedColor *color, const char *flavorName,
                                     Cloud_FlavorAcknowledgementCallbackType callback)
{
    if (!isConnected) {
        return false;
    }

    const char *flavorColorName = NULL;
    if (color != NULL) {

        if (!Color_TryGetNameForColor(color, &flavorColorName)) {
            Log_Debug("ERROR: Cannot get name for color (%d, %d, %d)\n.", color->red ? 1 : 0,
                      color->green ? 1 : 0, color->blue ? 1 : 0);
            return false;
        }
    }

    flavorAckCallbackFunc = callback;
    SendDeviceTwinUpdate(flavorName, flavorColorName);
    return true;
}

static void HandleConnectionStatusChange(bool connected)
{
    if (connectionStatusCallbackFunc != NULL) {
        connectionStatusCallbackFunc(connected);
    } else {
        Log_Debug("WARNING: Cloud interface - no connection status callback registered\n");
    }

    isConnected = connected;
}

static void HandleSendTelemetryCallback(bool success, void *context)
{
    if (context == (void *)&sendTelemetryMessageIdentifier) {
        if (sendTelemetryCallbackFunc != NULL) {
            sendTelemetryCallbackFunc(success);
        } else {
            Log_Debug(
                "WARNING: Cloud interface - no callback registered for send telemetry response");
        }
    }
}

static void HandleDeviceTwinCallback(const char *content)
{
    JSON_Value *rootProperties = NULL;
    rootProperties = json_parse_string(content);
    if (rootProperties == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
        goto cleanup;
    }

    JSON_Object *rootObject = json_value_get_object(rootProperties);
    JSON_Object *desiredProperties = json_object_get_object(rootObject, "desired");
    if (desiredProperties == NULL) {
        desiredProperties = rootObject;
    }

    // The desired properties should have a "NextFlavor" object
    JSON_Object *nextFlavor = json_object_get_object(desiredProperties, "NextFlavor");
    if (nextFlavor != NULL) {

        const char *flavorName = json_object_dotget_string(nextFlavor, "Name");
        const char *flavorColor = json_object_dotget_string(nextFlavor, "Color");

        if (flavorColor != NULL) {
            if (flavorName == NULL) {
                Log_Debug("INFO: Requested color: %s (no change in flavor)\n", flavorColor);
            } else {
                Log_Debug("INFO: Requested flavor and color: %s (%s)\n", flavorName, flavorColor);
            }

            LedColor color;
            if (Color_TryGetColorByName(flavorColor, &color)) {
                if (flavorReceivedCallbackFunc != NULL) {
                    flavorReceivedCallbackFunc(&color, flavorName);
                } else {
                    Log_Debug("WARNING: Cloud interface - no LED color callback registered\n");
                }
            } else {
                Log_Debug("ERROR: Cloud interface - unknown LED color '%s' in device twin\n",
                          flavorColor);
            }
        } else {
            if (flavorName == NULL) {
                Log_Debug("INFO: No change in requested color or name\n");
            } else {
                Log_Debug("INFO: Requested flavor: %s (no change in color)\n", flavorName);
            }

            if (flavorReceivedCallbackFunc != NULL) {
                flavorReceivedCallbackFunc(NULL, flavorName);
            } else {
                Log_Debug("WARNING: Cloud interface - no LED color callback registered\n");
            }
        }
    } else {
        Log_Debug(
            "WARNING: Cloud interface - reported device twin did not contain a NextFlavor desired "
            "property\n");
    }

cleanup:
    // Release the allocated memory.
    if (rootProperties != NULL) {
        json_value_free(rootProperties);
    }
}

static void SendDeviceTwinUpdate(const char *flavorName, const char *flavorColor)
{
    JSON_Value *twinStateValue = NULL;
    twinStateValue = json_value_init_object();
    JSON_Object *twinStateRoot = json_value_get_object(twinStateValue);
    if (flavorName != NULL) {
        json_object_dotset_string(twinStateRoot, "NextFlavor.Name", flavorName);
    }
    if (flavorColor != NULL) {
        json_object_dotset_string(twinStateRoot, "NextFlavor.Color", flavorColor);
    }

    char *serializedTwinState = json_serialize_to_string(twinStateValue);
    AzureIoT_DeviceTwinReportState(serializedTwinState,
                                   (void *)&acknowledgeFlavorMessageIdentifier);
    json_free_serialized_string(serializedTwinState);

    if (twinStateValue != NULL) {
        json_value_free(twinStateValue);
    }
}

static void HandleDeviceTwinUpdateAckCallback(bool success, void *context)
{

    if (context == &acknowledgeFlavorMessageIdentifier) {
        if (flavorAckCallbackFunc != NULL) {
            flavorAckCallbackFunc(success);
        } else {
            Log_Debug("WARNING: Cloud - no flavour ack call back handler registered\n");
        }
    } else {
        Log_Debug("WARNING: Cloud - unexpected device twin ack received\n");
    }
}
