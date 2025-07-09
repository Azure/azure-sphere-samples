/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "devicecontrol_message_protocol.h"
#include "devicecontrol_message_protocol_defs.h"
#include "message_protocol.h"
#include "applibs_versions.h"
#include <applibs/log.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

static DeviceControlMessageProtocol_SetLedStatusHandlerType setLedStatusHandler = NULL;
static DeviceControlMessageProtocol_GetLedStatusHandlerType getLedStatusHandler = NULL;
static bool getDesiredLedStatusRequestNeeded;
static bool reportLedStatusRequestNeeded;

static void ReportLedStatus(void);
static void GetDesiredLedStatusResponseHandler(MessageProtocol_CategoryId categoryId,
                                               MessageProtocol_RequestId requestId,
                                               const uint8_t *data, size_t dataSize,
                                               MessageProtocol_ResponseResult result, bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Get Desired LED status\" response.\n");
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Get Desired LED status\" failed with error code: %u.\n", result);
        return;
    }

    if (dataSize != sizeof(DeviceControlMessageProtocol_LedStatusStruct)) {
        Log_Debug("INFO: \"Get Desired LED status\" response is invalid.\n");
        return;
    }
    Log_Debug("INFO: \"Get Desired LED status\" succeeded.\n");

    DeviceControlMessageProtocol_LedStatusStruct *desiredLedStatus =
        (DeviceControlMessageProtocol_LedStatusStruct *)data;

    setLedStatusHandler(desiredLedStatus->status == 0x01);
    ReportLedStatus();
}

static void ReportLedStatusResponseHandler(MessageProtocol_CategoryId categoryId,
                                           MessageProtocol_RequestId requestId, const uint8_t *data,
                                           size_t dataSize, MessageProtocol_ResponseResult result,
                                           bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Report LED Status\" response.\n");
        return;
    }

    // This response contains no data, so check its result to see whether the request was successful
    if (result != 0) {
        Log_Debug("ERROR: \"Report LED Status\" failed with error code: %u.\n", result);
        return;
    }
    Log_Debug("INFO: \"Report LED Status\" succeeded.\n");
}

static void SendGetDesiredLedStatusRequest(void)
{
    getDesiredLedStatusRequestNeeded = false;

    // Send a "Get Desired LED Status" request
    Log_Debug("INFO: Sending request: \"Get Desired LED status\".\n");
    MessageProtocol_SendRequest(MessageProtocol_DeviceControlCategoryId,
                                DeviceControlMessageProtocol_GetDesiredLedStatusRequestId, NULL, 0,
                                &GetDesiredLedStatusResponseHandler);
}

static void DesiredLedStatusAvailableEventHandler(MessageProtocol_CategoryId categoryId,
                                                  MessageProtocol_EventId eventId)
{
    Log_Debug("INFO: Handling event: \"Desired LED Status Available\".\n");
    if (MessageProtocol_IsIdle()) {
        SendGetDesiredLedStatusRequest();
    } else {
        getDesiredLedStatusRequestNeeded = true;
    }
}

static void SendReportLedStatusRequest(void)
{
    reportLedStatusRequestNeeded = false;

    // Get the current LED status and set it in ledStatus
    DeviceControlMessageProtocol_LedStatusStruct ledStatus;
    memset(&ledStatus, 0, sizeof(ledStatus));
    ledStatus.status = getLedStatusHandler() ? 0x01 : 0x00;

    // Send a "Report LED Status" request
    Log_Debug("INFO: Sending request: \"Report LED Status\" with value %u.\n", ledStatus.status);
    MessageProtocol_SendRequest(MessageProtocol_DeviceControlCategoryId,
                                DeviceControlMessageProtocol_ReportLedStatusRequestId,
                                (const uint8_t *)&ledStatus, sizeof(ledStatus),
                                &ReportLedStatusResponseHandler);
}

static void ReportLedStatus(void)
{
    if (MessageProtocol_IsIdle()) {
        SendReportLedStatusRequest();
    } else {
        reportLedStatusRequestNeeded = true;
    }
}

static void LedStatusNeededEventHandler(MessageProtocol_CategoryId categoryId,
                                        MessageProtocol_EventId eventId)
{
    Log_Debug("INFO: Handling event: \"LED Status Needed\".\n");
    ReportLedStatus();
}

static void IdleHandler(void)
{
    if (getDesiredLedStatusRequestNeeded) {
        SendGetDesiredLedStatusRequest();
        return;
    }
    if (reportLedStatusRequestNeeded) {
        SendReportLedStatusRequest();
        return;
    }
}

void DeviceControlMessageProtocol_Init(
    DeviceControlMessageProtocol_SetLedStatusHandlerType setHandler,
    DeviceControlMessageProtocol_GetLedStatusHandlerType getHandler)
{
    setLedStatusHandler = setHandler;
    getLedStatusHandler = getHandler;

    // Register event handlers
    MessageProtocol_RegisterEventHandler(
        MessageProtocol_DeviceControlCategoryId,
        DeviceControlMessageProtocol_DesiredLedStatusAvailableEventId,
        DesiredLedStatusAvailableEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_DeviceControlCategoryId,
                                         DeviceControlMessageProtocol_LedStatusNeededEventId,
                                         LedStatusNeededEventHandler);

    // Register idle handler
    MessageProtocol_RegisterIdleHandler(IdleHandler);

    // Initialize event pending flags
    getDesiredLedStatusRequestNeeded = false;
    reportLedStatusRequestNeeded = false;
}

void DeviceControlMessageProtocol_Cleanup(void) {}

void DeviceControlMessageProtocol_NotifyLedStatusChange(void)
{
    Log_Debug("INFO: Notify LED status change.\n");
    ReportLedStatus();
}