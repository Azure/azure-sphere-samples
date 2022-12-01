/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdlib.h>
#include <stdbool.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>
#include <applibs/networking.h>

#include "parson.h"

#include "azure_iot.h"
#include "iothub.h"
#include "eventloop_timer_utilities.h"
#include "exitcodes.h"
#include "connection.h"

static void AzureIoTConnectTimerEventHandler(EventLoopTimer *timer);
static void AzureIoTDoWorkTimerEventHandler(EventLoopTimer *timer);
static void SetUpAzureIoTHubClient(void);
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                     IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                     void *userContextCallback);
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context);
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback);
static void ReportedStateCallback(int result, void *context);
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback);
static IOTHUBMESSAGE_DISPOSITION_RESULT CloudToDeviceCallback(IOTHUB_MESSAGE_HANDLE msg,
                                                              void *context);

static void ConnectionCallbackHandler(Connection_Status status,
                                      IOTHUB_DEVICE_CLIENT_LL_HANDLE clientHandle);
static bool IsConnectionReadyToSendTelemetry(void);

/// <summary>
/// Authentication state of the client with respect to the Azure IoT Hub.
/// </summary>
typedef enum {
    /// <summary>Client is not authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_NotAuthenticated = 0,
    /// <summary>Client has initiated authentication to the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_AuthenticationInitiated = 1,
    /// <summary>Client is authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_Authenticated = 2
} IoTHubClientAuthenticationState;

static IoTHubClientAuthenticationState iotHubClientAuthenticationState =
    IoTHubClientAuthenticationState_NotAuthenticated; // Authentication state with respect to the
                                                      // IoT Hub.

// Azure IoT poll periods
static const int AzureIoTDefaultConnectPeriodSeconds =
    1; // check if device is connected to the internet and Azure client is setup every second
static const int AzureIoTMinReconnectPeriodSeconds = 10;      // back off when reconnecting
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60; // back off limit
static const int AzureIoTDoWorkIntervalMilliseconds =
    100; // Call IoTHubDeviceClient_LL_DoWork() every 100 ms
static const int NanosecondsPerMillisecond = 1000000;
static int azureIoTConnectPeriodSeconds = -1;
static bool azureIoTInitialized = false;
static EventLoopTimer *azureIoTConnectionTimer = NULL;
static EventLoopTimer *azureIoTDoWorkTimer = NULL;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static ExitCode_CallbackType failureCallbackFunction = NULL;
static AzureIoT_Callbacks callbacks;

static Connection_Status connectionStatus = Connection_NotStarted;

// Constants
#define MAX_DEVICE_TWIN_PAYLOAD_SIZE 512

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS_REASON,
                                       IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

ExitCode AzureIoT_Initialize(EventLoop *eventLoop, ExitCode_CallbackType failureCallback,
                             const char *modelId, void *connectionContext, AzureIoT_Callbacks cb)
{
    if (AzureIoT_IsInitialized())
        return ExitCode_Success;

    failureCallbackFunction = failureCallback;
    callbacks = cb;

    ExitCode connectionErrorCode = Connection_Initialise(
        eventLoop, ConnectionCallbackHandler, failureCallback, modelId, connectionContext);
    if (connectionErrorCode != ExitCode_Success) {
        return connectionErrorCode;
    }

    azureIoTConnectPeriodSeconds = AzureIoTDefaultConnectPeriodSeconds;
    struct timespec azureIoTConnectPeriod = {.tv_sec = azureIoTConnectPeriodSeconds, .tv_nsec = 0};
    azureIoTConnectionTimer = CreateEventLoopPeriodicTimer(
        eventLoop, &AzureIoTConnectTimerEventHandler, &azureIoTConnectPeriod);
    if (azureIoTConnectionTimer == NULL) {
        return ExitCode_Init_AzureIoTConnectionTimer;
    }

    int azureIoTDoWorkIntervalNanoseconds =
        AzureIoTDoWorkIntervalMilliseconds * NanosecondsPerMillisecond;
    struct timespec azureIoTDoWorkPollPeriod = {.tv_sec = 0,
                                                .tv_nsec = azureIoTDoWorkIntervalNanoseconds};
    azureIoTDoWorkTimer = CreateEventLoopPeriodicTimer(eventLoop, &AzureIoTDoWorkTimerEventHandler,
                                                       &azureIoTDoWorkPollPeriod);
    if (azureIoTDoWorkTimer == NULL) {
        return ExitCode_Init_AzureIoTDoWorkTimer;
    }

    azureIoTInitialized = true;

    return ExitCode_Success;
}

void AzureIoT_Cleanup(void)
{
    DisposeEventLoopTimer(azureIoTConnectionTimer);
    DisposeEventLoopTimer(azureIoTDoWorkTimer);
}

/// <summary>
///     Called by the Azure IoT Hub connection code to indicate a change in connection status.
/// </summary>
/// <param name="status">Connection status.</param>
/// <param name="clientHandle">
///     If successfully connected to an Azure IoT Hub, this will be
///     set to a device client handle for that hub; otherwise, NULL.
/// </param>
static void ConnectionCallbackHandler(Connection_Status status,
                                      IOTHUB_DEVICE_CLIENT_LL_HANDLE clientHandle)
{
    connectionStatus = status;

    switch (status) {
    case Connection_NotStarted:
        break;
    case Connection_Started:
        Log_Debug("INFO: Azure IoT Hub connection started.\n");
        break;
    case Connection_Complete: {
        Log_Debug("INFO: Azure IoT Hub connection complete.\n");

        iothubClientHandle = clientHandle;

        // Successfully connected, so make sure the polling frequency is back to the default
        azureIoTConnectPeriodSeconds = AzureIoTDefaultConnectPeriodSeconds;
        struct timespec azureIoTConnectPeriod = {.tv_sec = azureIoTConnectPeriodSeconds,
                                                 .tv_nsec = 0};
        SetEventLoopTimerPeriod(azureIoTConnectionTimer, &azureIoTConnectPeriod);

        // Set client authentication state to initiated. This is done to indicate that
        // SetUpAzureIoTHubClient() has been called (and so should not be called again) while the
        // client is waiting for a response via the ConnectionStatusCallback().
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_AuthenticationInitiated;

        IoTHubDeviceClient_LL_SetMessageCallback(iothubClientHandle, CloudToDeviceCallback, NULL);
        IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, DeviceTwinCallback, NULL);
        IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, DeviceMethodCallback,
                                                      NULL);
        IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle,
                                                          ConnectionStatusCallback, NULL);
        break;
    }

    case Connection_Failed: {

        // If we fail to connect, reduce the polling frequency, starting at
        // AzureIoTMinReconnectPeriodSeconds and with a backoff up to
        // AzureIoTMaxReconnectPeriodSeconds
        if (azureIoTConnectPeriodSeconds == AzureIoTDefaultConnectPeriodSeconds) {
            azureIoTConnectPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
        } else {
            azureIoTConnectPeriodSeconds *= 2;
            if (azureIoTConnectPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
                azureIoTConnectPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
            }
        }

        struct timespec azureIoTConnectPeriod = {.tv_sec = azureIoTConnectPeriodSeconds,
                                                 .tv_nsec = 0};
        SetEventLoopTimerPeriod(azureIoTConnectionTimer, &azureIoTConnectPeriod);

        Log_Debug("ERROR: Azure IoT Hub connection failed - will retry in %i seconds.\n",
                  azureIoTConnectPeriodSeconds);
        break;
    }
    }
}

/// <summary>
///     Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureIoTConnectTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        failureCallbackFunction(ExitCode_AzureIoTConnectionTimer_Consume);
        return;
    }

    // Check whether the network is up.
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady &&
            (iotHubClientAuthenticationState == IoTHubClientAuthenticationState_NotAuthenticated)) {
            SetUpAzureIoTHubClient();
        }
    } else {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        failureCallbackFunction(ExitCode_IsNetworkingReady_Failed);
        return;
    }
}

/// <summary>
///     azureIoTDoWorkTimer timer event:  Call IoTHubDeviceClient_LL_DoWork()
/// </summary>
static void AzureIoTDoWorkTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        failureCallbackFunction(ExitCode_AzureIoTDoWorkTimer_Consume);
        return;
    }

    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetUpAzureIoTHubClient(void)
{
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
        iothubClientHandle = NULL;
    }

    if (connectionStatus == Connection_NotStarted || connectionStatus == Connection_Failed) {
        Connection_Start();
    }
}

/// <summary>
///     Callback when the Azure IoT connection state changes.
///     This can indicate that a new connection attempt has succeeded or failed.
///     It can also indicate than an existing connection has expired due to SAS token expiry.
/// </summary>
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                     IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                     void *userContextCallback)
{
    Log_Debug("Azure IoT connection status: %s\n",
              IOTHUB_CLIENT_CONNECTION_STATUS_REASONStrings(reason));

    iotHubClientAuthenticationState = result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED
                                          ? IoTHubClientAuthenticationState_Authenticated
                                          : IoTHubClientAuthenticationState_NotAuthenticated;

    if (iotHubClientAuthenticationState == IoTHubClientAuthenticationState_NotAuthenticated) {
        ConnectionCallbackHandler(Connection_NotStarted, NULL);
    }

    if (callbacks.connectionStatusCallbackFunction != NULL) {
        callbacks.connectionStatusCallbackFunction(result ==
                                                   IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
    }
}

/// <summary>
///     Callback invoked when a Device Twin update is received from Azure IoT Hub.
/// </summary>
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback)
{
    // Statically allocate this for more predictable memory use patterns
    static char nullTerminatedJsonString[MAX_DEVICE_TWIN_PAYLOAD_SIZE + 1];

    if (payloadSize > MAX_DEVICE_TWIN_PAYLOAD_SIZE) {
        Log_Debug("ERROR: Device twin payload size (%u bytes) exceeds maximum (%u bytes).\n",
                  payloadSize, MAX_DEVICE_TWIN_PAYLOAD_SIZE);

        failureCallbackFunction(ExitCode_PayloadSize_TooLarge);
        return;
    }

    // Copy the payload to local buffer for null-termination.
    memcpy(nullTerminatedJsonString, payload, payloadSize);

    // Add the null terminator at the end.
    nullTerminatedJsonString[payloadSize] = 0;

    if (callbacks.deviceTwinReceivedCallbackFunction != NULL) {
        callbacks.deviceTwinReceivedCallbackFunction(nullTerminatedJsonString);
    }
}

/// <summary>
///     Callback invoked when a message is received from IoTHub
/// </summary>
static IOTHUBMESSAGE_DISPOSITION_RESULT CloudToDeviceCallback(IOTHUB_MESSAGE_HANDLE msg,
                                                              void *context)
{
    if (callbacks.cloudToDeviceCallbackFunction != NULL) {
        callbacks.cloudToDeviceCallbackFunction(msg);
    }

    return IOTHUB_MESSAGE_OK;
}

AzureIoT_Result AzureIoT_SetCallbacks(AzureIoT_Callbacks cbs)
{
    if (cbs.cloudToDeviceCallbackFunction)
        callbacks.cloudToDeviceCallbackFunction = cbs.cloudToDeviceCallbackFunction;

    if (cbs.connectionStatusCallbackFunction)
        callbacks.connectionStatusCallbackFunction = cbs.connectionStatusCallbackFunction;

    if (cbs.deviceMethodCallbackFunction)
        callbacks.deviceMethodCallbackFunction = cbs.deviceMethodCallbackFunction;

    if (cbs.deviceTwinReceivedCallbackFunction)
        callbacks.deviceTwinReceivedCallbackFunction = cbs.deviceTwinReceivedCallbackFunction;

    if (cbs.deviceTwinReportStateAckCallbackTypeFunction)
        callbacks.deviceTwinReportStateAckCallbackTypeFunction =
            cbs.deviceTwinReportStateAckCallbackTypeFunction;

    if (cbs.sendTelemetryCallbackFunction)
        callbacks.sendTelemetryCallbackFunction = cbs.sendTelemetryCallbackFunction;

    return AzureIoT_Result_OK;
}

AzureIoT_Result AzureIoT_ClearCallbacks(void)
{
    AzureIoT_Callbacks blank = {NULL};
    callbacks = blank;

    return AzureIoT_Result_OK;
}

AzureIoT_Result AzureIoT_SendTelemetry(const char *jsonMessage, const char *iso8601DateTimeString,
                                       void *context)
{
    Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", jsonMessage);

    // Check whether the device is connected to the internet.
    if (IsConnectionReadyToSendTelemetry() == false) {
        return AzureIoT_Result_NoNetwork;
    }

    if (iotHubClientAuthenticationState != IoTHubClientAuthenticationState_Authenticated) {
        // AzureIoT client is not authenticated. Log a warning and return.
        Log_Debug("WARNING: Azure IoT Hub is not authenticated. Not sending telemetry.\n");
        return AzureIoT_Result_OtherFailure;
    }

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonMessage);

    if (messageHandle == 0) {
        Log_Debug("ERROR: unable to create a new IoTHubMessage.\n");
        return AzureIoT_Result_OtherFailure;
    }

    if (iso8601DateTimeString != NULL) {
        IoTHubMessage_SetProperty(messageHandle, "iothub-creation-time-utc", iso8601DateTimeString);
    }

    AzureIoT_Result result = AzureIoT_Result_OK;

    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendEventCallback,
                                             context) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure requesting IoTHubClient to send telemetry event.\n");
        result = AzureIoT_Result_OtherFailure;
    } else {
        Log_Debug("INFO: IoTHubClient accepted the telemetry event for delivery.\n");
    }

    IoTHubMessage_Destroy(messageHandle);
    return result;
}

/// <summary>
///     Callback invoked when the Azure IoT Hub send event request is processed.
/// </summary>
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub send telemetry event callback: status code %d.\n", result);

    if (callbacks.sendTelemetryCallbackFunction != NULL) {
        callbacks.sendTelemetryCallbackFunction(result == IOTHUB_CLIENT_CONFIRMATION_OK, context);
    }
}

/// <summary>
///     Enqueues a report containing Device Twin reported properties. The report is not sent
///     immediately, but it is sent on the next invocation of IoTHubDeviceClient_LL_DoWork().
/// </summary>
AzureIoT_Result AzureIoT_DeviceTwinReportState(const char *jsonState, void *context)
{
    if (IsConnectionReadyToSendTelemetry() == false) {
        return AzureIoT_Result_NoNetwork;
    }

    if (iotHubClientAuthenticationState != IoTHubClientAuthenticationState_Authenticated) {
        // AzureIoT client is not authenticated. Log a warning and return.
        Log_Debug("WARNING: Azure IoT Hub is not authenticated. Not sending device twin.\n");
        return AzureIoT_Result_OtherFailure;
    }

    if (IoTHubDeviceClient_LL_SendReportedState(
            iothubClientHandle, (const unsigned char *)jsonState, strlen(jsonState),
            ReportedStateCallback, context) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Azure IoT Hub client error when reporting state '%s'.\n", jsonState);
        return AzureIoT_Result_OtherFailure;
    }

    Log_Debug("INFO: Azure IoT Hub client accepted request to report state '%s'.\n", jsonState);
    return AzureIoT_Result_OK;
}

bool AzureIoT_IsInitialized(void)
{
    return azureIoTInitialized;
}

/// <summary>
///     Callback invoked when the Device Twin report state request is processed by Azure IoT Hub
///     client.
/// </summary>
static void ReportedStateCallback(int result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub Device Twin reported state callback: status code %d.\n", result);

    if (callbacks.deviceTwinReportStateAckCallbackTypeFunction != NULL) {
        callbacks.deviceTwinReportStateAckCallbackTypeFunction(result != 0, context);
    }
}

/// <summary>
///     Callback invoked when a Direct Method is received from Azure IoT Hub.
/// </summary>
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback)
{
    int result = -1;

    Log_Debug("Received Device Method callback: Method name %s.\n", methodName);

    if (callbacks.deviceMethodCallbackFunction != NULL) {
        result = callbacks.deviceMethodCallbackFunction(methodName, payload, payloadSize, response,
                                                        responseSize);
    }

    return result;
}

/// <summary>
///     Check the network status.
/// </summary>
static bool IsConnectionReadyToSendTelemetry(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        failureCallbackFunction(ExitCode_IsNetworkingReady_Failed);
        return false;
    }

    if (!isNetworkReady) {
        Log_Debug("WARNING: Cannot send Azure IoT Hub telemetry because the network is not up.\n");
    }
    return isNetworkReady;
}

bool AzureIoT_IsConnected(void)
{
    return iotHubClientAuthenticationState == IoTHubClientAuthenticationState_Authenticated;
}