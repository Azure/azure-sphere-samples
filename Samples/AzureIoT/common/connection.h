/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>

#include <azureiot/iothub_device_client_ll.h>

#include "exitcodes.h"

// This header defines an interface for establishing a connection to Azure IoT.
//
// Implementations specific to each connection type (IoTHub, DPS and IoTEdge) can be found
// in the corresponding directory.

/// <summary>
/// Status of the connection attempt.
/// </summary>
typedef enum {
    Connection_NotStarted,
    Connection_Started,
    Connection_Complete,
    Connection_Failed
} Connection_Status;

/// <summary>
/// A callback type for a function to be invoked when connection status changes, following a call
/// to <see cref="Connection_Start" />
/// </summary>
/// <param name="status">Connection status</param>
/// <param name="iothubDeviceClientHandle">If <paramref name="status" /> is
/// <see cref="Connection_Complete" />, this will contain a valid
/// <see cref="IOTHUB_DEVICE_CLIENT_LL_HANDLE" />. Otherwise, NULL.</param>
typedef void (*Connection_StatusCallbackType)(
    Connection_Status status, IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubDeviceClientHandle);

/// <summary>
/// Initialize (but do not start) connection to an Azure IoT Hub. Requires
/// implementation-specific context data retrieved from
/// <see cref="Options_GetConnectionContext" />.
/// </summary>
/// <param name="el">Pointer to an EventLoop to which events can be registered.</param>
/// <param name="statusCallBack">Function to be called when connection status changes.</param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <param name="modelId">Azure IoT PnP model ID, as a NULL-terminated string.</param>
/// <param name="context">Implementation-specific context data required for connection.</param>
/// <returns></returns>
ExitCode Connection_Initialise(EventLoop *el, Connection_StatusCallbackType statusCallBack,
                               ExitCode_CallbackType failureCallback, const char *modelId,
                               void *context);

/// <summary>
/// Begin connection to an Azure IoT Hub.
///
/// On successful connection, the status callback passed to
/// <see cref="Connection_Initialize" /> will be invoked with an
/// <see cref="IOTHUB_DEVICE_CLIENT_LL_HANDLE" /> and the <see cref="Connection_Status" />
/// status. On failure, the status callback will be invoked with
/// <see cref="Connection_Failed" />.
///
/// This is an asynchronous operation; it is not guaranteed that this function will return before
/// the connection status callback is called.
/// </summary>
void Connection_Start(void);

/// <summary>
/// Close and cleanup any resources needed by the Azure IoT Hub connection.
/// </summary>
void Connection_Cleanup(void);
