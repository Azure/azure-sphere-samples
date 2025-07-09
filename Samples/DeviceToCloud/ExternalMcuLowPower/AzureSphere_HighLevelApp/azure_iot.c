/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#include <stdlib.h>
#include <errno.h>

#include "applibs_versions.h"
#include <applibs/application.h>
#include <applibs/networking.h>
#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "eventloop_timer_utilities.h"

// Azure IoT SDK
#include <azureiot/iothub_client_core_common.h>
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothub_client_options.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azureiot/iothub.h>
#include <azureiot/azure_sphere_provisioning.h>
#include <azure_prov_client/iothub_security_factory.h>

#include "exitcode.h"
#include "azure_iot.h"

// Constants
#define MAX_DEVICE_TWIN_PAYLOAD_SIZE 512

// Azure IoT definitions.
static const size_t MAX_SCOPEID_LENGTH = 16;
static char *idScope = NULL;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubAuthenticated = false;

// Function declarations
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context);
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback);
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                     IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                     void *userContextCallback);
static void ReportedStateCallback(int result, void *context);
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback);
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);
static void SetupAzureClient(void);
static void AzureTimerEventHandler(EventLoopTimer *timer);
static bool SetupAzureIoTHubClientWithDps(void);
static bool IsConnectionReadyToSendTelemetry(void);

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *azureTimer = NULL;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 1;        // poll azure iot every second
static const int AzureIoTMinReconnectPeriodSeconds = 5;       // back off when reconnecting
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60; // back off limit

static int azureIoTPollPeriodSeconds = -1;

static ExitCodeCallbackType exitCodeCallbackFunction = NULL;
static AzureIoT_ConnectionStatusCallbackType connectionStatusCallbackFunc = NULL;
static AzureIoT_DeviceTwinReceivedCallbackType deviceTwinReceivedCallbackFunc = NULL;
static AzureIoT_SendTelemetryCallbackType sendTelemetryCallbackFunc = NULL;
static AzureIoT_DeviceTwinReportStateAckCallbackType deviceTwinReportStateAckCallbackFunc = NULL;

ExitCode AzureIoT_Initialize(
    EventLoop *el, const char *scopeId, ExitCodeCallbackType failureCallback,
    AzureIoT_ConnectionStatusCallbackType connectionStatusCallback,
    AzureIoT_DeviceTwinReceivedCallbackType deviceTwinReceivedCallback,
    AzureIoT_SendTelemetryCallbackType sendTelemetryCallback,
    AzureIoT_DeviceTwinReportStateAckCallbackType deviceTwinReportStateAckCallback)
{
    if (scopeId == NULL) {
        return ExitCode_AzureIoT_Init_InvalidScopeId;
    }

    exitCodeCallbackFunction = failureCallback;
    eventLoop = el;
    idScope = strndup(scopeId, MAX_SCOPEID_LENGTH);
    if (idScope == NULL) {
        return ExitCode_Init_CopyScopeId;
    }

    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL) {
        return ExitCode_Init_AzureTimer;
    }

    connectionStatusCallbackFunc = connectionStatusCallback;
    deviceTwinReceivedCallbackFunc = deviceTwinReceivedCallback;
    deviceTwinReportStateAckCallbackFunc = deviceTwinReportStateAckCallback;
    sendTelemetryCallbackFunc = sendTelemetryCallback;

    return ExitCode_Success;
}

void AzureIoT_Cleanup(void)
{
    free(idScope);
    DisposeEventLoopTimer(azureTimer);
}

/// <summary>
///     Azure timer event: Connect if necessary, do Azure IoT work
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Could not consume Azure IoT timer event\n");
        if (exitCodeCallbackFunction != NULL) {
            exitCodeCallbackFunction(ExitCode_AzureTimer_Consume);
        }
        return;
    }

    // Check whether the network is up.
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady && !iothubAuthenticated) {
            SetupAzureClient();
        }
    } else {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCodeCallbackFunction(ExitCode_IsNetworkingReady_Failed);
        return;
    }

    if (iothubAuthenticated) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetupAzureClient(void)
{
    bool isAzureClientSetupSuccessful = false;

    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
    }

    bool deviceAuthReady = false;
    if (Application_IsDeviceAuthReady(&deviceAuthReady) == 0 && deviceAuthReady) {
        isAzureClientSetupSuccessful = SetupAzureIoTHubClientWithDps();
        if (!isAzureClientSetupSuccessful) {
            Log_Debug("WARNING: Failed to create IoTHub handle");
        }
    } else {
        Log_Debug("WARNING: Device auth not ready");
    }

    if (!isAzureClientSetupSuccessful) {
        // If we fail to connect, reduce the polling frequency, starting at
        // AzureIoTMinReconnectPeriodSeconds and with a backoff up to
        // AzureIoTMaxReconnectPeriodSeconds
        if (azureIoTPollPeriodSeconds == AzureIoTDefaultPollPeriodSeconds) {
            azureIoTPollPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
        } else {
            azureIoTPollPeriodSeconds *= 2;
            if (azureIoTPollPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
                azureIoTPollPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
            }
        }

        struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
        SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

        Log_Debug(" - will retry in %i seconds.\n", azureIoTPollPeriodSeconds);
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

    iothubAuthenticated = true;

    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
                                        &keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"%s\".\n",
                  OPTION_KEEP_ALIVE);
        return;
    }

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, DeviceTwinCallback, NULL);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, DeviceMethodCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle, ConnectionStatusCallback,
                                                      NULL);
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DPS
/// </summary>
static bool SetupAzureIoTHubClientWithDps(void)
{
    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(idScope, 10000,
                                                                          &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
              GetAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        return false;
    }

    return true;
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
    iothubAuthenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
    Log_Debug("Azure IoT connection status: %s\n", GetReasonString(reason));

    if (connectionStatusCallbackFunc != NULL) {
        connectionStatusCallbackFunc(iothubAuthenticated);
    } else {
        Log_Debug("WARNING: Azure IoT - no connection status callback handler registered\n");
    }
}

/// <summary>
///     Callback invoked when a Direct Method is received from Azure IoT Hub.
/// </summary>
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback)
{
    // All methods are ignored
    int result = -1;
    char *responseString = "{}";

    Log_Debug("Received Device Method callback: Method name %s.\n", methodName);

    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
    *responseSize = strlen(responseString);
    *response = malloc(*responseSize);
    memcpy(*response, responseString, *responseSize);
    return result;
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
        return;
    }

    // Copy the payload to local buffer for null-termination.
    memcpy(nullTerminatedJsonString, payload, payloadSize);

    // Add the null terminator at the end.
    nullTerminatedJsonString[payloadSize] = 0;

    Log_Debug(nullTerminatedJsonString);
    Log_Debug("\n");

    if (deviceTwinReceivedCallbackFunc != NULL) {
        deviceTwinReceivedCallbackFunc(nullTerminatedJsonString);
    }
}

/// <summary>
///     Converts the Azure IoT Hub connection status reason to a string.
/// </summary>
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

void AzureIoT_SendTelemetry(const char *jsonMessage, void *context)
{
    Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", jsonMessage);

    // Check whether the device is connected to the internet.
    if (IsConnectionReadyToSendTelemetry() == false) {
        return;
    }

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonMessage);

    if (messageHandle == 0) {
        Log_Debug("ERROR: unable to create a new IoTHubMessage.\n");
        return;
    }

    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendEventCallback,
                                             context) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure requesting IoTHubClient to send telemetry event.\n");
    } else {
        Log_Debug("INFO: IoTHubClient accepted the telemetry event for delivery.\n");
    }

    IoTHubMessage_Destroy(messageHandle);
}

/// <summary>
///     Check the network status.
/// </summary>
static bool IsConnectionReadyToSendTelemetry(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCodeCallbackFunction(ExitCode_IsNetworkingReady_Failed);
        return false;
    }

    if (!isNetworkReady) {
        Log_Debug("WARNING: Cannot send Azure IoT Hub telemetry because the network is not up.\n");
    }
    return isNetworkReady;
}

/// <summary>
///     Callback invoked when the Azure IoT Hub send event request is processed.
/// </summary>
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub send telemetry event callback: status code %d.\n", result);
    if (sendTelemetryCallbackFunc != NULL) {
        sendTelemetryCallbackFunc(result == IOTHUB_CLIENT_CONFIRMATION_OK, context);
    } else {
        Log_Debug("WARNING: No callback handler registered for send telemetry event\n");
    }
}

void AzureIoT_DeviceTwinReportState(const char *jsonState, void *context)
{
    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: Azure IoT Hub client not initialized.\n");
    } else {
        if (IoTHubDeviceClient_LL_SendReportedState(
                iothubClientHandle, (const unsigned char *)jsonState, strlen(jsonState),
                ReportedStateCallback, context) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: Azure IoT Hub client error when reporting state '%s'.\n", jsonState);
        } else {
            Log_Debug("INFO: Azure IoT Hub client accepted request to report state '%s'.\n",
                      jsonState);
        }
    }
}

/// <summary>
///     Callback invoked when the Device Twin report state request is processed by Azure IoT Hub
///     client.
/// </summary>
static void ReportedStateCallback(int result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub device twin reported state: %d\n", result);
    if (deviceTwinReportStateAckCallbackFunc != NULL) {
        deviceTwinReportStateAckCallbackFunc(true, context);
    } else {
        Log_Debug("WARNING: No callback handler for a device twin ack.\n");
    }
}
