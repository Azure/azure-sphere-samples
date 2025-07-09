/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "telemetry.h"
#include "color.h"

typedef void (*McuMessagingFailureCallbackType)(void);

/// <summary>
///     Initialize comms with the external MCU.
/// </summary>
void McuMessaging_Initialize(void);

typedef void (*McuMessagingInitCallbackType)(void);

/// <summary>
///     Send an init message to the MCU. On receipt of a successful response, call
///     <paramref="successCallback" />; on failure, call <paramref="failureCallback" />.
/// </summary>
/// <param name="successCallback">Function to call on receipt of a successful response.</param>
/// <param name="failCallback">Function to call if no response is received.</param>
void McuMessaging_Init(McuMessagingInitCallbackType successCallback,
                       McuMessagingFailureCallbackType failureCallback);

typedef void (*McuMessagingRequestTelemetryCallbackType)(const DeviceTelemetry *telemetry);

/// <summary>
///     Send a request for telemetry to the MCU. On receipt of a successful response, call
///     <paramref="successCallback" /> with the telemetry data; on failure, call
///     <paramref="failureCallback" />.
/// </summary>
/// <param name="successCallback">Function to call on successful receipt of telemetry.</param>
/// <param name="failCallback">Function to call if no response is received.</param>
void McuMessaging_RequestTelemetry(McuMessagingRequestTelemetryCallbackType successCallback,
                                   McuMessagingFailureCallbackType failureCallback);

typedef void (*McuMessagingSetLedCallbackType)(const LedColor *color);

/// <summary>
///     Send a request to set the LED color to the MCU. On receipt of a successful response, call
///     <paramref="successCallback" />; on failure, call <paramref="failureCallback" />.
/// </summary>
/// <param name="color">LED color to set</param>
/// <param name="successCallback">Function to call on successful receipt of SetLed response.</param>
/// <param name="failCallback">Function to call if no response is received.</param>
void McuMessaging_SetLed(const LedColor *color, McuMessagingSetLedCallbackType successCallback,
                         McuMessagingFailureCallbackType failureCallback);
