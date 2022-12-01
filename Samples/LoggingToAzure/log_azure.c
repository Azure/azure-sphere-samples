/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "parson.h"
#include "log_azure.h"

#define _GNU_SOURCE // enable vasprintf
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

static int cloudLogEnabled = 1;
static int cloudLogInitialized = 0;

void Log_Azure_C2D_Message_Received(IOTHUB_MESSAGE_HANDLE message)
{
    // The message will be free'd by IoT-C-SDK.
    IOTHUBMESSAGE_CONTENT_TYPE messageType = IoTHubMessage_GetContentType(message);
    char *decodedMessage = NULL;

    if (messageType == IOTHUBMESSAGE_BYTEARRAY) {
        const char *encoding = IoTHubMessage_GetContentEncodingSystemProperty(message);

        // if an encoding is specified and it is not utf-8, error.
        if (encoding && strncmp(encoding, "utf-8", 5) != 0) {
            Log_Debug("[C2D] Only UTF-8 encoded strings are supported. Ignoring C2D message.\n");
            return;
        }

        // otherwise we assume that the encoding is utf-8
        size_t length = 0;
        const unsigned char *messageBytes = NULL;
        IOTHUB_MESSAGE_RESULT result = IoTHubMessage_GetByteArray(message, &messageBytes, &length);

        if (result == IOTHUB_MESSAGE_OK) {
            decodedMessage = (char *)malloc(length + 1);
            memcpy(decodedMessage, messageBytes, length);
            decodedMessage[length] = 0;
        } else
            Log_Debug("[C2D] Decoding error: %d\n", result);
    } else if (messageType == IOTHUBMESSAGE_STRING) {
        decodedMessage = (char *)IoTHubMessage_GetString(message);
    }

    Log_Debug("IoTHub message received: %s, type: %d\n", decodedMessage, messageType);

    //  Accepted JSON Object:
    //  {
    //      "configureDebug": {
    //          "enabled": bool
    //      }
    //  }
    if (decodedMessage) {
        JSON_Value *v = json_parse_string(decodedMessage);
        JSON_Object *obj = json_value_get_object(v);

        if (v && obj) {
            JSON_Object *configureDebug = json_object_dotget_object(obj, "configureDebug");

            int c2dEnableLogging = json_object_dotget_boolean(configureDebug, "enabled");

            if (c2dEnableLogging > -1) {
                Log_Debug("[C2D] Cloud logging %s\n",
                          (c2dEnableLogging == 0) ? "disabled" : "enabled");
                cloudLogEnabled = c2dEnableLogging;
            }
        } else
            Log_Debug("'%s' is not valid or expected JSON", decodedMessage);

        json_value_free(v);
    }

    if (messageType == IOTHUBMESSAGE_BYTEARRAY)
        free(decodedMessage);
}

static AzureIoT_Result Log_Azure_Internal(char *log)
{
    Log_Debug("[D2C] Sending: %s\n", log);
    JSON_Value *telemetryValue = json_value_init_object();
    JSON_Object *telemetryRoot = json_value_get_object(telemetryValue);

    json_object_dotset_string(telemetryRoot, "debugMessage", log);

    char *serializedTelemetry = json_serialize_to_string(telemetryValue);
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(serializedTelemetry, NULL, NULL);

    json_free_serialized_string(serializedTelemetry);
    json_value_free(telemetryValue);

    return aziotResult;
}

void Log_Azure_Init(bool override_callback)
{
    if (cloudLogInitialized)
        return;

    if (override_callback) {
        AzureIoT_Callbacks callbacks = {NULL};
        callbacks.cloudToDeviceCallbackFunction = Log_Azure_C2D_Message_Received;
        AzureIoT_SetCallbacks(callbacks);
    }

    cloudLogInitialized = 1;
}

void Log_AzureSetEnabled(bool enabled)
{
    cloudLogEnabled = enabled;
}

AzureIoT_Result Log_Azure(const char *fmt, ...)
{
    if (!AzureIoT_IsInitialized()) {
        Log_Debug("AzureIoT not initialized.\n");
        return AzureIoT_Result_OtherFailure;
    }

    if (!AzureIoT_IsConnected()) {
        Log_Debug("No network.\n");
        return AzureIoT_Result_NoNetwork;
    }

    if (cloudLogEnabled == 0) {
        Log_Debug("Cloud logging is disabled.\n");
        return AzureIoT_Result_OtherFailure;
    }

    Log_Azure_Init(true);

    Log_Debug("Attempting message send.\n");

    va_list args;
    va_start(args, fmt);
    char *formatted_string;
    int length = vasprintf(&formatted_string, fmt, args);

    int result = AzureIoT_Result_OtherFailure;

    if (length != -1) {
        result = Log_Azure_Internal(formatted_string);
        free(formatted_string);
    }

    va_end(args);

    return result;
}
