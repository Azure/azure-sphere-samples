/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "exitcodes.h"
#include "eventloop_timer_utilities.h"
#include "azure_iot.h"
#include <sys/time.h>
#include "../avnet/uart_support.h"
#if defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    
#include "linkedList.h"
#endif 
#define ARGS_PER_TELEMETRY_ITEM 3
#define ARGS_PER_TWIN_ITEM 3

void SendTelemetryTimerEventHandler(EventLoopTimer *timer);
void GetCurrentTime(char* returnBuffer);

// This header describes a backend-agnostic interface to a cloud platform.
// An implementation of this header should implement logic to translate between business domain
// concepts and the specifics of the cloud backend.

/// <summary>
/// Callback type for a function to be invoked when the cloud backend requests and alert be
/// displayed.
/// </summary>
/// <param name="alertMessage">A NULL-terminated string containing the alert message.</param>
typedef void (*Cloud_DisplayAlertCallbackType)(const char *alertMessage);

/// <summary>
/// Callback type for a function to be invoked when the cloud backend indicates the connection
/// status has changed.
/// </summary>
/// <param name="connected">A boolean indicating whether the cloud connection is available.</param>
typedef void (*Cloud_ConnectionChangedCallbackType)(bool connected);

/// <summary>
/// An enum indicating possible result codes when performing cloud-related operations
/// </summary>
typedef enum {
    /// <summary>
    /// The operation succeeded
    /// </summary>
    Cloud_Result_OK = 0,

    /// <summary>
    /// The operation could not be performed as no network connection was available
    /// </summary>
    Cloud_Result_NoNetwork,

    /// <summary>
    /// The operation could not be performed as the device was not authenticated to the IoTHub
    /// </summary>
    Cloud_Result_NotAuthenticated,

    /// <summary>
    /// The operation could not be performed as the device was associated to a device template in IoT Connect
    /// </summary>
    Cloud_Result_IoTConnect_unassociated,

    /// <summary>
    /// The operation could not be performed as the message send to the IoTHub failed
    /// </summary>
    Cloud_SendFailed,

    /// <summary>
    /// The operation failed for another reason not explicitly listed
    /// </summary>
    Cloud_Result_OtherFailure
} Cloud_Result;

/// <summary>
/// Initialize the cloud connection.
/// </summary>
/// <param name="el">An EventLoop that events may be registered to.</param>
/// <param name="backendContext">
///     Backend-specific context to be passed to the backend during initialization.
/// </param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <param name="displayAlertCallback">
///     Function called when the cloud backend requests an alert be displayed.
/// </param>
/// <param name="connectionChangedCallback">
///     Function called when the status of the connection to the cloud backend changes.
/// </param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode Cloud_Initialize(
    EventLoop *el, void *backendContext, ExitCode_CallbackType failureCallback,
    Cloud_DisplayAlertCallbackType displayAlertCallback,
    Cloud_ConnectionChangedCallbackType connectionChangedCallback);

/// <summary>
/// Disconnect and cleanup the cloud connection.
/// </summary>
void Cloud_Cleanup(void);

/// <summary>
/// Queue sending telemtry to the cloud backend.
/// </summary>
/// <param name="telemetry">A pointer to a <see cref="Cloud_Telemetry" /> structure to send.</param>
/// <returns>A <see cref="Cloud_Result" /> indicating success or failure.</returns>
//Cloud_Result Cloud_SendTelemetry(const Cloud_Telemetry *telemetry);
Cloud_Result Cloud_SendTelemetry(bool IoTConnectFormat, int arg_count, ...);

/// <summary>
/// Queue sending device details to the cloud
/// </summary>
/// <param name="serialNumber">The device's serial number</param>
/// <returns>A <see cref="Cloud_Result" /> indicating success or failure.</returns>
Cloud_Result Cloud_SendReadOnlyDeviceTwinStrings(int arg_count, ...);

// Utility functions
//Cloud_Result Cloud_SendDeviceTwinUpdate(bool ioTPnPFormat, int arg_count, ...);
Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result);

const char *CloudResultToString(Cloud_Result result);

