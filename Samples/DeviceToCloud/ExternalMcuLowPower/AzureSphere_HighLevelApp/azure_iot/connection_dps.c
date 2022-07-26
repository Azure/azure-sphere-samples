/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include <applibs/application.h>
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/networking.h>

// Azure IoT SDK
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothub_client_options.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azure_prov_client/prov_device_ll_client.h>
#include <azure_prov_client/iothub_security_factory.h>
#include <azure_prov_client/prov_security_factory.h>
#include <azure_prov_client/prov_transport_mqtt_client.h>
#include <azure_c_shared_utility/shared_util_options.h>

#include <azureiot/azure_sphere_provisioning.h>

#include "exitcodes.h"
#include "eventloop_timer_utilities.h"
#include "connection.h"
#include "connection_dps.h"

static void InitializeProvisioningClient(void);
static void CleanupProvisioningClient(void);
static bool IsReadyToProvision(void);
static void RegisterDeviceCallback(PROV_DEVICE_RESULT registerResult, const char *callbackHubUri,
                                   const char *deviceId, void *userContext);
static void ProvisioningTimerHandler(EventLoopTimer *timer);
static void TimeoutTimerHandler(EventLoopTimer *timer);
static void OnRegisterComplete(void);

static ExitCode_CallbackType failureCallbackFunction = NULL;
static Connection_StatusCallbackType connectionStatusCallback = NULL;

static bool dpsRegisterCompleted = false;
static PROV_DEVICE_RESULT dpsRegisterStatus = PROV_DEVICE_RESULT_INVALID_STATE;

#define MAX_HUB_URI_LENGTH (512)
#define MAX_SCOPEID_LENGTH (32)
#define MAX_MODELID_LENGTH (512)
#define MAX_DTDL_BUFFER_SIZE \
    (15 + MAX_MODELID_LENGTH + 1) // 15 chars is the length of '{"modelId":""}'

static char iotHubUri[MAX_HUB_URI_LENGTH + 1];
static char scopeId[MAX_SCOPEID_LENGTH + 1];
static char azureSphereModelId[MAX_MODELID_LENGTH + 1];

PROV_DEVICE_LL_HANDLE provHandle = NULL;
static const char dpsUrl[] = "global.azure-devices-provisioning.net";

static EventLoopTimer *provisioningTimer = NULL;
static EventLoopTimer *timeoutTimer = NULL;

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUE);

ExitCode Connection_Initialise(EventLoop *el, Connection_StatusCallbackType statusCallBack,
                               ExitCode_CallbackType failureCallback, const char *modelId,
                               void *context)
{
    failureCallbackFunction = failureCallback;
    connectionStatusCallback = statusCallBack;

    Connection_Dps_Config *config = context;

    if (NULL == config) {
        Log_Debug("ERROR: DPS connection context cannot be NULL.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (NULL == config->scopeId) {
        Log_Debug("ERROR: DPS connection config must specify an ID scope.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (strnlen(config->scopeId, MAX_SCOPEID_LENGTH) == MAX_SCOPEID_LENGTH) {
        Log_Debug("ERROR: ID scope length exceed maximum of %d\n", MAX_SCOPEID_LENGTH);
        return ExitCode_Validate_ConnectionConfig;
    }
    strncpy(scopeId, config->scopeId, MAX_SCOPEID_LENGTH);

    if (NULL != modelId) {
        if (strnlen(modelId, MAX_MODELID_LENGTH) == MAX_MODELID_LENGTH) {
            Log_Debug("ERROR: Model ID length exceeds maximum of %d\n", MAX_MODELID_LENGTH);
            return ExitCode_Validate_ConnectionConfig;
        }
        strncpy(azureSphereModelId, modelId, MAX_MODELID_LENGTH);
    } else {
        azureSphereModelId[0] = '\0';
    }

    provisioningTimer = CreateEventLoopDisarmedTimer(el, ProvisioningTimerHandler);

    if (provisioningTimer == NULL) {
        Log_Debug("ERROR: Failed to create provisioning event loop timer: %s (%d)\n",
                  strerror(errno), errno);
        return ExitCode_Connection_CreateTimer;
    }

    timeoutTimer = CreateEventLoopDisarmedTimer(el, TimeoutTimerHandler);

    if (timeoutTimer == NULL) {
        Log_Debug("ERROR: Failed to create provisioning timeout timer: %s (%d)\n", strerror(errno),
                  errno);
        return ExitCode_Connection_CreateTimer;
    }

    return ExitCode_Success;
}

void Connection_Start(void)
{
    static const long timeoutSec = 10;
    static const long workDelayMs = 25;

    dpsRegisterCompleted = false;
    dpsRegisterStatus = PROV_DEVICE_RESULT_INVALID_STATE;

    if (!IsReadyToProvision()) {
        // If we're not ready to provision, indicate that the connection has failed (which will
        // cause a retry)
        connectionStatusCallback(Connection_Failed, NULL);
        return;
    }

    InitializeProvisioningClient();
    if (provHandle == NULL) {
        Log_Debug("ERROR: Failed to create and initialize device provisioning client\n");
        failureCallbackFunction(ExitCode_Connection_InitializeClient);
        return;
    }

    const struct timespec sleepTime = {.tv_sec = 0, .tv_nsec = workDelayMs * 1000 * 1000};
    if (SetEventLoopTimerPeriod(provisioningTimer, &sleepTime) == -1) {
        Log_Debug("ERROR: Failed to start provisioning event loop timer: %s (%d)\n",
                  strerror(errno), errno);
        failureCallbackFunction(ExitCode_Connection_TimerStart);
        return;
    }

    const struct timespec timeoutTime = {.tv_sec = timeoutSec, .tv_nsec = 0};
    if (SetEventLoopTimerOneShot(timeoutTimer, &timeoutTime) == -1) {
        Log_Debug("ERROR: Failed to start provisioning timeout timer: %s (%d)\n", strerror(errno),
                  errno);
        failureCallbackFunction(ExitCode_Connection_TimerStart);
        return;
    }

    connectionStatusCallback(Connection_Started, NULL);
}

static void ProvisioningTimerHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Failed to consume provisioning timer event: %s (%d)\n", strerror(errno),
                  errno);
        failureCallbackFunction(ExitCode_Connection_TimerConsume);
        return;
    }

    if (provHandle != NULL) {
        Prov_Device_LL_DoWork(provHandle);
    }

    if (dpsRegisterCompleted) {
        DisarmEventLoopTimer(timer);
        DisarmEventLoopTimer(timeoutTimer);
        OnRegisterComplete();
    }
}

static void TimeoutTimerHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Failed to consume provisioning timeout timer event: %s (%d)\n",
                  strerror(errno), errno);
        failureCallbackFunction(ExitCode_Connection_TimerConsume);
        return;
    }

    DisarmEventLoopTimer(timer);
    DisarmEventLoopTimer(provisioningTimer);
    CleanupProvisioningClient();

    Log_Debug("ERROR: Timed out waiting for device provisioning service to provision device\n");
    connectionStatusCallback(Connection_Failed, NULL);
}

/// <summary>
///     Callback that gets called on device registration for provisioning
/// </summary>
static void RegisterDeviceCallback(PROV_DEVICE_RESULT registerResult, const char *callbackHubUri,
                                   const char *deviceId, void *userContext)
{
    dpsRegisterCompleted = true;
    dpsRegisterStatus = registerResult;

    if (registerResult == PROV_DEVICE_RESULT_OK) {

        Log_Debug("INFO: DPS device registration successful\n");

        if (callbackHubUri != NULL) {
            size_t uriSize = strlen(callbackHubUri);
            if (uriSize > MAX_HUB_URI_LENGTH) {
                Log_Debug("ERROR: IoT Hub URI size (%u bytes) exceeds maximum (%u bytes).\n",
                          uriSize, MAX_HUB_URI_LENGTH);
                return;
            }
            strncpy(iotHubUri, callbackHubUri, strlen(callbackHubUri));
        } else {
            Log_Debug("ERROR: Device registration did not return an IoT Hub URI\n");
        }
    }
}

static bool IsReadyToProvision(void)
{
    bool networkingReady = false;

    // Verifies networking on device
    if (Networking_IsNetworkingReady(&networkingReady) != 0) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!networkingReady) {
        Log_Debug("ERROR: DPS connection - networking not ready.\n");
        return false;
    }

    // Verifies authentication is ready on device
    bool currentAppDeviceAuthReady = false;
    if (Application_IsDeviceAuthReady(&currentAppDeviceAuthReady) != 0) {
        Log_Debug("ERROR: Application_IsDeviceAuthReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!currentAppDeviceAuthReady) {
        Log_Debug("ERROR: DPS connection - device auth not ready.\n");
        return false;
    }

    return true;
}

static void InitializeProvisioningClient(void)
{
    static char dtdlBuffer[MAX_DTDL_BUFFER_SIZE];

    int len =
        snprintf(dtdlBuffer, MAX_DTDL_BUFFER_SIZE, "{\"modelId\":\"%s\"}", azureSphereModelId);
    if (len < 0 || len >= MAX_DTDL_BUFFER_SIZE) {
        Log_Debug("ERROR: Cannot write Model ID to buffer.\n");
        return;
    }

    // Initiate security with X509 Certificate
    if (prov_dev_security_init(SECURE_DEVICE_TYPE_X509) != 0) {
        Log_Debug("ERROR: Failed to initiate X509 Certificate security\n");
        goto cleanup;
    }

    // Create Provisioning Client for communication with DPS
    // using MQTT protocol
    provHandle = Prov_Device_LL_Create(dpsUrl, scopeId, Prov_Device_MQTT_Protocol);
    if (provHandle == NULL) {
        Log_Debug("ERROR: Failed to create Provisioning Client\n");
        goto cleanup;
    }

    // Use DAA cert in provisioning flow - requires the SetDeviceId option to be set on the
    // provisioning client.
    static const int deviceIdForDaaCertUsage = 1;
    PROV_DEVICE_RESULT prov_result =
        Prov_Device_LL_SetOption(provHandle, "SetDeviceId", &deviceIdForDaaCertUsage);
    if (prov_result != PROV_DEVICE_RESULT_OK) {
        Log_Debug("ERROR: Failed to set Device ID in Provisioning Client\n");
        goto cleanup;
    }

    // Sets Model ID provisioning data
    prov_result = Prov_Device_LL_Set_Provisioning_Payload(provHandle, dtdlBuffer);
    if (prov_result != PROV_DEVICE_RESULT_OK) {
        Log_Debug("Error: Failed to set Model ID in Provisioning Client\n");
        goto cleanup;
    }

    // Sets the callback function for device registration
    prov_result =
        Prov_Device_LL_Register_Device(provHandle, RegisterDeviceCallback, NULL, NULL, NULL);
    if (prov_result != PROV_DEVICE_RESULT_OK) {
        Log_Debug("ERROR: Failed to set callback function for device registration\n");
        goto cleanup;
    }

    return;

cleanup:
    CleanupProvisioningClient();
}

static void CleanupProvisioningClient()
{
    if (provHandle != NULL) {
        Prov_Device_LL_Destroy(provHandle);
        provHandle = NULL;
    }

    prov_dev_security_deinit();
}

static void OnRegisterComplete(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;

    if (dpsRegisterStatus != PROV_DEVICE_RESULT_OK) {
        Log_Debug("ERROR: Failed to register device with provisioning service: %s\n",
                  PROV_DEVICE_RESULTStrings(dpsRegisterStatus));
        goto cleanup;
    }

    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(iotHubUri, &MQTT_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: Failed to create client IoT Hub Client Handle\n");
        goto cleanup;
    }

    // Use DAA cert when connecting - requires the SetDeviceId option to be set on the
    // IoT Hub client.
    static const int deviceIdForDaaCertUsage = 1;
    IOTHUB_CLIENT_RESULT iothubResult = IoTHubDeviceClient_LL_SetOption(
        iothubClientHandle, "SetDeviceId", &deviceIdForDaaCertUsage);
    if (iothubResult != IOTHUB_CLIENT_OK) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
        iothubClientHandle = NULL;
        Log_Debug("ERROR: Failed to set Device ID on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        goto cleanup;
    }

    // Sets auto URL encoding on IoT Hub Client
    static bool urlAutoEncodeDecode = true;
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
             iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) !=
        IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set auto Url encode option on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        goto cleanup;
    }

    // Sets model ID on IoT Hub Client
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_MODEL_ID,
                                                        azureSphereModelId)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set the Model ID on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        goto cleanup;
    }

cleanup:
    if (iothubClientHandle != NULL) {
        connectionStatusCallback(Connection_Complete, iothubClientHandle);
    } else {
        connectionStatusCallback(Connection_Failed, NULL);
    }

    CleanupProvisioningClient();
}

void Connection_Cleanup(void) {}
