/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "azure_iot.h"

/// <summary>
/// Defines functions relevant to logging generic debug messages to AzureIoT.
/// Upon the first call to Log_Azure, initialization overrides the cloudToDeviceMessage
/// callback of AzureIoT. The callback allows logging to be enabled/disabled remotely
/// using cloud-to-device messages.
///
/// The cloud-to-device message should contain json that matches the following structure:
///
/// {
///     "configureDebug": {
///         "enabled": bool
///     }
/// }
///
/// If you use cloud-to-device messages in your application already, you can implement a
/// custom handler that multiplexes cloud-to-device requests and invokes
/// Log_Azure_C2D_Message_Received. To disable cloudToDeviceMessage callback overriding, call
/// Log_Azure_Init(false) before any calls to Log_Azure.
/// </summary>

/// <summary>
///     Log data to Azure
/// </summary>
/// <param name="fmt_string">A conventional printf format string</param>
/// <param name="...">variables to log into the format string</param>
/// <returns>An <see cref="AzureIoT_Result" /> indicating success or failure.</returns>
AzureIoT_Result Log_Azure(const char *fmt_string, ...);

/// <summary>
///     Enables/disables logging to Azure.
/// </summary>
/// <param name="enabled">true to enable, false to disable</param>
void Log_AzureSetEnabled(bool enabled);

/// <summary>
///     Explicitly initialize Azure logging.
///     Required if cloud-to-device messages are already
///     used in your application.
///
///     Note: It is advisable to multiplex messages to Log_Azure_C2D_Message_Received
///     if cloud-to-device messages are already used by the application. This retains the
///     ability to toggle cloud logging from AzureIoT
/// </summary>
/// <param name="override_callback">true (default), false do not override the cloud-to-device
/// message callback handler</param>
void Log_Azure_Init(bool override_callback);

/// <summary>
///     The cloud-to-device message handler for Log_Azure.
///     You must call this if cloud-to-device messages are already used
///     in your application.
/// </summary>
/// <param name="message">The message received from AzureIoT.</param>
void Log_Azure_C2D_Message_Received(IOTHUB_MESSAGE_HANDLE message);