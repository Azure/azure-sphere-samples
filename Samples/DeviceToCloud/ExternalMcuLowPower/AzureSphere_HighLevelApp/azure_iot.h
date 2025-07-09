/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>
#include "exitcode.h"

typedef void (*AzureIoT_DeviceTwinReceivedCallbackType)(const char *deviceTwinContent);
typedef void (*AzureIoT_ConnectionStatusCallbackType)(bool connected);
typedef void (*AzureIoT_SendTelemetryCallbackType)(bool success, void *context);
typedef void (*AzureIoT_DeviceTwinReportStateAckCallbackType)(bool success, void *context);

/// <summary>
///     Initialize the Azure IoT Hub connection.
/// </summary>
/// <param name="el">EventLoop to register timers and events to.</param>
/// <param name="scopeId">The scope ID for the IoT Central app.</param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <param name="connectionStatusCallback">Function called on connection status change.</param>
/// <param name="deviceTwinReceivedCallback">
///     Function called when device twin update received.
/// </param>
/// <param name="sendTelemetryCallback">
///     Function called on receipt of telemetry by the Azure IoT Hub (or in case of failure).
/// </param>
/// <param name="deviceTwinSendUpdateAckCallback">
///     Function called on receipt of ack (or failure) from the Azure IoT Hub in response to sending
///     a device twin update.
/// </param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode AzureIoT_Initialize(
    EventLoop *el, const char *scopeId, ExitCodeCallbackType failureCallback,
    AzureIoT_ConnectionStatusCallbackType connectionStatusCallback,
    AzureIoT_DeviceTwinReceivedCallbackType deviceTwinReceivedCallback,
    AzureIoT_SendTelemetryCallbackType sendTelemetryCallback,
    AzureIoT_DeviceTwinReportStateAckCallbackType deviceTwinSendUpdateAckCallback);

/// <summary>
///     Closes and cleans up the Azure IoT Hub connection
/// </summary>
void AzureIoT_Cleanup(void);

/// <summary>
///     Enqueue telemetry to send to the Azure IoT Hub. The telemetry is not sent immediately; the
///     function will return immediately, and then call the
///     <see cref="AzureIoT_SendTelemetryCallbackType" /> (passed to
///     <see cref="AzureIoT_Initialize" />) to indicate success or failure.
/// </summary>
/// <param name="jsonMessage">The telemetry to send, as a JSON string.</param>
/// <param name="context">An optional context, which will be passed to the callback.</param>
void AzureIoT_SendTelemetry(const char *jsonMessage, void *context);

/// <summary>
///     Enqueue a report containing Device Twin properties to send to the Azure IoT Hub. The report
///     is not sent immediately; the function will return immediately, and then call the
///     <see cref="AzureIoT_DeviceTwinReportStateAckCallbackType" /> (passed to
///     <see cref="AzureIoT_Initialize" />) to indicate success or failure.
/// </summary>
/// <param name="jsonState">A JSON string representing the device twin properties to report.</param>
/// <param name="context">An optional context, which will be passed to the callback.</param>
void AzureIoT_DeviceTwinReportState(const char *jsonState, void *context);
