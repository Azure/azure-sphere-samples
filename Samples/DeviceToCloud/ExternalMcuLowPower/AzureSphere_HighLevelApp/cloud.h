/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// This header defines a backend-agnostic interface for publishing telemetry to and receiving
// state from a cloud service. The implementation of this interface will be specific for a
// particular cloud service (for the purposes of this reference solution, this is Azure IoT Central)

#include <applibs/eventloop.h>

#include "color.h"
#include "exitcodes.h"
#include "telemetry.h"

typedef void (*Cloud_FlavorReceivedCallbackType)(const LedColor *color, const char *flavorName);
typedef void (*Cloud_SendTelemetryCallbackType)(bool success);
typedef void (*Cloud_FlavorAcknowledgementCallbackType)(bool success);
typedef void (*Cloud_ConnectionStatusCallbackType)(bool connected);

/// <summary>
///     Initialise the cloud connection.
/// </summary>
/// <param name="el">The application EventLoop, for registering timers and events.</param>
/// <param name="backendConfiguration">Backend-specific data required for initialization.</param>
/// <param name="failureCallback">A function to be called in the event of failures.</param>
/// <param name="connectionStatusCallback">
///     A <see cref="Cloud_ConnectionStatusCallbackType" /> to be called on connection status
///     change.
/// </param>
/// <param name="flavorReceivedCallback">
///     A <see cref="Cloud_FlavorReceivedCallbackType" /> to be called on receipt of a new flavor
///     from the cloud.
///</param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode Cloud_Initialize(EventLoop *el, void *backendConfiguration,
                          ExitCode_CallbackType failureCallback,
                          Cloud_ConnectionStatusCallbackType connectionStatusCallback,
                          Cloud_FlavorReceivedCallbackType flavorReceivedCallback);

/// <summary>
///     Close and cleanup the cloud connection.
/// </summary>
void Cloud_Cleanup(void);

/// <summary>
///     Queue telemetry for sending to the cloud; returns a Boolean indicating whether the telemetry
///     could be queued for sending. If the telemetry was successfully queued,
///     <paramref name="callback" /> will be invoked asynchronously to indicate successful receipt
///     (or otherwise) by the cloud.
/// </summary>
/// <param name="telemetry">Pointer to telemetry to send.</param>
/// <param name="callback">
///     A <see cref="Cloud_SendTelemetryCallbackType" /> to be invoked, to indicate whether the
///     telemetry was successfully received by the cloud backend.
/// </param>
/// <returns>
///     A Boolean indicating whether the telemetry was successfuly queued for sending.
/// </returns>
bool Cloud_SendTelemetry(const CloudTelemetry *telemetry, Cloud_SendTelemetryCallbackType callback);

/// <summary>
///     Queue a message to the cloud acknowledging the new flavor sent to the device. Should be
///     called in response to <see cref="Cloud_FlavorReceivedCallbackType" />. Returns a Boolean
///     indicating whether the message could be queued. If the message was successfully queued,
///     <paramref name="callback" /> will be invoked asynchronously to indicate successful receipt
///     (or failure) by the cloud.
/// </summary>
/// <param name="color">LED color being acknowledged (or NULL if not specified)</param>
/// <param name="flavorName">Name of the flavor being acknowledge (or NULL if not specified)</param>
/// <param name="callback">
///     A <see cref="Cloud_AcknowledgementCallbackType" /> to be invoked, to indicate whether the
///     message was successfully receieved by the cloud backend.
/// </param>
/// <returns>
///     A Boolean indicating whether the message was successfully queued for sending.
/// </returns>
bool Cloud_SendFlavorAcknowledgement(const LedColor *color, const char *flavorName,
                                     Cloud_FlavorAcknowledgementCallbackType callback);
