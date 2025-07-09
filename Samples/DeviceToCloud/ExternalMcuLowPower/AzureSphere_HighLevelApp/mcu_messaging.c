/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "eventloop_timer_utilities.h"
#include "message_protocol.h"
#include "messages.h"

#include "mcu_messaging.h"

static McuMessagingInitCallbackType initCallback = NULL;
static McuMessagingRequestTelemetryCallbackType requestTelemetryCallback = NULL;
static McuMessagingSetLedCallbackType setLedCallback = NULL;
static McuMessagingFailureCallbackType failCallback = NULL;

void McuMessaging_Initialize(void) {}

static bool CheckResponse(const char *responseName, MessageProtocol_CategoryId expectedCategory,
                          MessageProtocol_CategoryId actualCategory,
                          MessageProtocol_RequestId expectedRequest,
                          MessageProtocol_RequestId actualRequest, size_t expectedSize,
                          size_t actualSize, bool timedOut)
{
    bool failed = false;

    if (timedOut) {
        Log_Debug("ERROR: %s response - timed out waiting for response\n", responseName);
        failed = true;
    } else {
        if (actualCategory != expectedCategory) {
            Log_Debug("ERROR: %s response - invalid category ID '%u' (expected '%u')", responseName,
                      expectedCategory, actualCategory);
            failed = true;
        }

        if (actualRequest != expectedRequest) {
            Log_Debug("ERROR: %s response - invalid request ID '%u' (expected '%u')", responseName,
                      expectedRequest, actualRequest);
            failed = true;
        }

        if (expectedSize != actualSize) {
            Log_Debug("ERROR: %s response - invalid body size %u bytes (expected %u bytes)",
                      responseName, expectedSize, actualSize);
            failed = true;
        }
    }

    return failed;
}

static void InitResponseHandler(MessageProtocol_CategoryId categoryId,
                                MessageProtocol_RequestId requestId, const uint8_t *data,
                                size_t dataSize, MessageProtocol_ResponseResult result,
                                bool timedOut)
{
    bool failed = CheckResponse("Init", MessageProtocol_McuToCloud_CategoryId, categoryId,
                                MessageProtocol_McuToCloud_Init, requestId,
                                sizeof(MessageProtocol_McuToCloud_InitStruct), dataSize, timedOut);

    if (!failed) {
        MessageProtocol_McuToCloud_InitStruct *initStruct =
            (MessageProtocol_McuToCloud_InitStruct *)data;

        if (initStruct->protocolVersion != MessageProtocol_McuToCloud_ProtocolVersion) {
            Log_Debug("ERROR: Protocol version mismatch (expected %u, received %u)\n",
                      initStruct->protocolVersion, MessageProtocol_McuToCloud_ProtocolVersion);
            failed = true;
        }
    }

    if (failed) {
        if (failCallback != NULL) {
            failCallback();
        } else {
            Log_Debug("ERROR: No failure handler registered.");
        }
        return;
    }

    if (initCallback != NULL) {
        initCallback();
    } else {
        Log_Debug("WARNING: Init response - no handler registered.");
    }
}

void McuMessaging_Init(McuMessagingInitCallbackType successCallback,
                       McuMessagingFailureCallbackType failureCallback)
{
    initCallback = successCallback;
    failCallback = failureCallback;

    MessageProtocol_SendRequest(MessageProtocol_McuToCloud_CategoryId,
                                MessageProtocol_McuToCloud_Init, NULL, 0, InitResponseHandler);
}

static void TelemetryResponseHandler(MessageProtocol_CategoryId categoryId,
                                     MessageProtocol_RequestId requestId, const uint8_t *data,
                                     size_t dataSize, MessageProtocol_ResponseResult result,
                                     bool timedOut)
{
    bool failed =
        CheckResponse("RequestTelemtry", MessageProtocol_McuToCloud_CategoryId, categoryId,
                      MessageProtocol_McuToCloud_RequestTelemetry, requestId,
                      sizeof(MessageProtocol_McuToCloud_TelemetryStruct), dataSize, timedOut);

    if (failed) {
        if (failCallback != NULL) {
            failCallback();
        } else {
            Log_Debug("ERROR: No failure handler registered.");
        }
    } else {
        if (requestTelemetryCallback != NULL) {
            MessageProtocol_McuToCloud_TelemetryStruct *receivedTelemetry =
                (MessageProtocol_McuToCloud_TelemetryStruct *)data;

            DeviceTelemetry telemetry = {
                .lifetimeTotalDispenses = receivedTelemetry->lifetimeTotalDispenses,
                .lifetimeTotalStockedDispenses = receivedTelemetry->lifetimeTotalStockedDispenses,
                .capacity = receivedTelemetry->capacity,
                .batteryLevel = receivedTelemetry->batteryLevel};

            requestTelemetryCallback(&telemetry);

        } else {
            Log_Debug("WARNING: RequestTelemetry response - no handler registered.");
        }
    }
}

void McuMessaging_RequestTelemetry(McuMessagingRequestTelemetryCallbackType successCallback,
                                   McuMessagingFailureCallbackType failureCallback)
{
    Log_Debug("INFO: Requesting telemetry from MCU\n");
    requestTelemetryCallback = successCallback;
    failCallback = failureCallback;

    MessageProtocol_SendRequest(MessageProtocol_McuToCloud_CategoryId,
                                MessageProtocol_McuToCloud_RequestTelemetry, NULL, 0,
                                TelemetryResponseHandler);
}

static void SetLedResponseHandler(MessageProtocol_CategoryId categoryId,
                                  MessageProtocol_RequestId requestId, const uint8_t *data,
                                  size_t dataSize, MessageProtocol_ResponseResult result,
                                  bool timedOut)
{
    bool failed =
        CheckResponse("SetLed", MessageProtocol_McuToCloud_CategoryId, categoryId,
                      MessageProtocol_McuToCloud_SetLed, requestId,
                      sizeof(MessageProtocol_McuToCloud_SetLedStruct), dataSize, timedOut);

    if (failed) {
        if (failCallback != NULL) {
            failCallback();
        } else {
            Log_Debug("ERROR: No failure handler registered.");
        }
    } else {
        if (setLedCallback != NULL) {
            MessageProtocol_McuToCloud_SetLedStruct *setLed =
                (MessageProtocol_McuToCloud_SetLedStruct *)data;
            LedColor color = {setLed->red != 0, setLed->green != 0, setLed->blue != 0};

            setLedCallback(&color);
        } else {
            Log_Debug("WARNING: SetLed response - no handler registered.");
        }
    }
}

void McuMessaging_SetLed(const LedColor *color, McuMessagingSetLedCallbackType successCallback,
                         McuMessagingFailureCallbackType failureCallback)
{
    MessageProtocol_McuToCloud_SetLedStruct leds = {.red = color->red ? 0xff : 0x00,
                                                    .green = color->green ? 0xff : 0x00,
                                                    .blue = color->blue ? 0xff : 0x00,
                                                    .reserved = 0};

    setLedCallback = successCallback;
    failCallback = failureCallback;

    MessageProtocol_SendRequest(MessageProtocol_McuToCloud_CategoryId,
                                MessageProtocol_McuToCloud_SetLed, (const uint8_t *)&leds,
                                sizeof(leds), SetLedResponseHandler);
}
