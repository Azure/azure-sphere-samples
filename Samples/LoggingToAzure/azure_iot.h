/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>
#include "exitcodes.h"
#include "iothub_message.h"

/// <summary>
/// Callback type for a function to be invoked on IoT Hub connection status change.
/// </summary>
/// <param name="connected">A boolean indicating connection status.</param>
typedef void (*AzureIoT_ConnectionStatusCallbackType)(bool connected);

/// <summary>
/// Callback type for a function to be invoked when telemetry is sent to the IoT Hub, or when a
/// send attempt fails.
/// </summary>
/// <param name="success">A boolean indicating whether the telemetry was successfully sent.</param>
/// <param name="context">The user context supplied when the request to send telemetry was
/// queued, if any.</param>
typedef void (*AzureIoT_SendTelemetryCallbackType)(bool success, void *context);

/// <summary>
/// Callback type for a function to be invoked when a device twin message is received.
/// </summary>
/// <param name="deviceTwinContent">The device twin, as a NULL-terminated JSON string.</param>
typedef void (*AzureIoT_DeviceTwinReceivedCallbackType)(const char *deviceTwinContent);

/// <summary>
/// Callback type for a function to be invoked when a device twin update is sent to the IoT Hub,
/// or when a send attempt fails.
/// </summary>
/// <param name="success">A boolean indicating whether the device twin update was successfully
/// sent.</param>
/// <param name="context">The user context supplied when the request to send the update was
/// queued, if any.</param>
typedef void (*AzureIoT_DeviceTwinReportStateAckCallbackType)(bool success, void *context);

/// <summary>
/// Callback type for a function to be invoked when a request to invoke a device method is
/// received from the IoT Hub. The method may return a response by setting
/// <paramref name="response" /> and <paramref name="responseSize" />.
/// </summary>
/// <param name="methodName">Name of the device method to invoke, as a NULL-terminated
/// string.</param>
/// <param name="payload">Payload for the method invocation, if any.</param>
/// <param name="payloadSize">Size of the payload.</param>
/// <param name="response">(out) pointer to a buffer containing the response.</param>
/// <param name="responseSize">(out) the size of the response buffer, if any.</param>
typedef int (*AzureIoT_DeviceMethodCallbackType)(const char *methodName,
                                                 const unsigned char *payload, size_t payloadSize,
                                                 unsigned char **response, size_t *responseSize);

/// <summary>
/// Callback function definition when a cloud-to-device message is received from IoTHub
/// <param name="msg">The cloud-to-device message provided by Azure IoT C SDK</param>
/// </summary>
typedef void (*AzureIoT_CloudToDeviceCallbackType)(IOTHUB_MESSAGE_HANDLE msg);

/// <summary>
/// Structure holding callback functions for Azure IoT Hub events
/// </summary>
typedef struct {
    /// <summary>
    /// Function called when the Azure IoT Hub connection status changes
    /// </summary>
    AzureIoT_ConnectionStatusCallbackType connectionStatusCallbackFunction;
    /// <summary>
    /// Function called when a Device Twin message is received from the Azure IoT Hub
    /// </summary>
    AzureIoT_DeviceTwinReceivedCallbackType deviceTwinReceivedCallbackFunction;
    /// <summary>
    /// Function called when the Azure IoT Hub acknowledges receipt of a Device Twin report
    /// </summary>
    AzureIoT_DeviceTwinReportStateAckCallbackType deviceTwinReportStateAckCallbackTypeFunction;
    /// <summary>
    /// Function called when a telemetry event is sent to the Azure IoT Hub
    /// </summary>
    AzureIoT_SendTelemetryCallbackType sendTelemetryCallbackFunction;
    /// <summary>
    /// Function called when the Azure IoT Hub invokes a device method
    /// </summary>
    AzureIoT_DeviceMethodCallbackType deviceMethodCallbackFunction;
    /// <summary>
    /// Function called when a cloud-to-device message is received from Azure IoT Hub
    /// </summary>
    AzureIoT_CloudToDeviceCallbackType cloudToDeviceCallbackFunction;
} AzureIoT_Callbacks;

/// <summary>
/// An enum indicating possible result codes when performing Azure IoT-related operations
/// </summary>
typedef enum {
    /// <summary>
    /// The operation succeeded
    /// </summary>
    AzureIoT_Result_OK = 0,

    /// <summary>
    /// The operation could not be performed as no network connection was available
    /// </summary>
    AzureIoT_Result_NoNetwork,

    /// <summary>
    /// The operation failed for another reason not explicitly listed
    /// </summary>
    AzureIoT_Result_OtherFailure
} AzureIoT_Result;

/// <summary>
///     Initialize the Azure IoT Hub connection.
/// </summary>
/// <param name="el">EventLoop to register timers and events to.</param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <param name="modelId">Azure IoT PnP model ID, if using; NULL otherwise.</param>
/// <param name="connectionContext">Implementation-specific context data for connection.</param>
/// <param name="callbacks">Structure holding callback functions for Azure IoT Hub events.</param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode AzureIoT_Initialize(EventLoop *el, ExitCode_CallbackType failureCallback,
                             const char *modelId, void *connectionContext,
                             AzureIoT_Callbacks callbacks);

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
/// <param name="iso8601DateTimeString">
///     Timestamp for the event as an ISO 8601 date/time string; if NULL, no timestamp will be
///     included with the message.
/// </param>
/// <param name="context">An optional context, which will be passed to the callback.</param>
/// <returns>An <see cref="AzureIoT_Result" /> indicating success or failure.</returns>
AzureIoT_Result AzureIoT_SendTelemetry(const char *jsonMessage, const char *iso8601DateTimeString,
                                       void *context);

/// <summary>
///     Enqueue a report containing Device Twin properties to send to the Azure IoT Hub. The report
///     is not sent immediately; the function will return immediately, and then call the
///     <see cref="AzureIoT_DeviceTwinReportStateAckCallbackType" /> (passed to
///     <see cref="AzureIoT_Initialize" />) to indicate success or failure.
/// </summary>
/// <param name="jsonState">A JSON string representing the device twin properties to report.</param>
/// <param name="context">An optional context, which will be passed to the callback.</param>
/// <returns>An <see cref="AzureIoT_Result" /> indicating success or failure.</returns>
AzureIoT_Result AzureIoT_DeviceTwinReportState(const char *jsonState, void *context);

/// <summary>
///     Updates the internal callback handlers to match those provided.
///     After invoking this function, the provided callbacks will be used instead.
///     To avoid updating a callback, set the callback handler to NULL in the provided
///     AzureIoT_Callbacks struct.
/// </summary>
/// <param name="callbacks">An AzureIoT_Callbacks struct with the required callbacks set.</param>
/// <returns>An <see cref="AzureIoT_Result" /> indicating success or failure.</returns>
AzureIoT_Result AzureIoT_SetCallbacks(AzureIoT_Callbacks callbacks);

/// <summary>
///     Clears the internal callback handlers.
/// </summary>
/// <returns>An <see cref="AzureIoT_Result" /> indicating success or failure.</returns>
AzureIoT_Result AzureIoT_ClearCallbacks(void);

/// <summary>
///     Determines whether AzureIoT_Initialize has been called.
/// </summary>
/// <returns>A boolean indicating initialization state.</returns>
bool AzureIoT_IsInitialized(void);

/// <summary>
///     Determines whether an authenticated connection to Azure IoT Hub has been established.
/// </summary>
/// <returns>A boolean indicating connection state.</returns>
bool AzureIoT_IsConnected(void);
