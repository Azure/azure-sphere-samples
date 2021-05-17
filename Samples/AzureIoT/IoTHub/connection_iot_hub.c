/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <applibs/application.h>
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/networking.h>

// Azure IoT SDK
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <azure_sphere_provisioning.h>
#include <azure_prov_client/iothub_security_factory.h>
#include <shared_util_options.h>

#include "exitcodes.h"

#include "connection.h"
#include "connection_iot_hub.h"

#define MAX_HOSTNAME_LENGTH (256)
#define MAX_MODELID_LENGTH (512)

static char hostname[MAX_HOSTNAME_LENGTH + 1];
static char azureSphereModelId[MAX_MODELID_LENGTH + 1];

IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;

static Connection_StatusCallbackType connectionStatusCallback = NULL;

static bool SetUpAzureIoTHubClientWithDaa(void);
static bool IsReadyToConnect(void);

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUE);

ExitCode Connection_Initialise(EventLoop *el, Connection_StatusCallbackType statusCallBack,
                               ExitCode_CallbackType failureCallback, const char *modelId,
                               void *context)
{
    (void)failureCallback;
    connectionStatusCallback = statusCallBack;

    if (NULL != modelId) {
        if (strnlen(modelId, MAX_MODELID_LENGTH) == MAX_MODELID_LENGTH) {
            Log_Debug("ERROR: Model ID length exceeds maximum of %d\n", MAX_MODELID_LENGTH);
            return ExitCode_Validate_ConnectionConfig;
        }
        strncpy(azureSphereModelId, modelId, MAX_MODELID_LENGTH);
    } else {
        azureSphereModelId[0] = '\0';
    }

    Connection_IotHub_Config *config = context;

    if (NULL == config) {
        Log_Debug("ERROR: IoT Hub connection context cannot be NULL.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (NULL == config->hubHostname) {
        Log_Debug("ERROR: IoT Hub connection config must specify a hostname.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (strnlen(config->hubHostname, MAX_HOSTNAME_LENGTH) == MAX_HOSTNAME_LENGTH) {
        Log_Debug("ERROR: Specified IoT hub hostname exceeds maximum length '%d'.\n",
                  MAX_HOSTNAME_LENGTH);
        return ExitCode_Validate_Hostname;
    }

    strncpy(hostname, config->hubHostname, MAX_HOSTNAME_LENGTH);

    return ExitCode_Success;
}

void Connection_Start(void)
{
    connectionStatusCallback(Connection_Started, NULL);

    if (SetUpAzureIoTHubClientWithDaa()) {
        connectionStatusCallback(Connection_Complete, iothubClientHandle);
    } else {
        connectionStatusCallback(Connection_Failed, NULL);
    }
}

void Connection_Cleanup(void) {}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DAA
/// </summary>
static bool SetUpAzureIoTHubClientWithDaa(void)
{
    bool retVal = true;

    // If network/DAA are not ready, fail out (which will trigger a retry)
    if (!IsReadyToConnect()) {
        return false;
    }

    // Set up auth type
    int retError = iothub_security_init(IOTHUB_SECURITY_TYPE_X509);
    if (retError != 0) {
        Log_Debug("ERROR: iothub_security_init failed with error %d.\n", retError);
        return false;
    }

    // Create Azure Iot Hub client handle
    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(hostname, MQTT_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: IoTHubDeviceClient_LL_CreateFromDeviceAuth returned NULL.\n");
        retVal = false;
        goto cleanup;
    }

    IOTHUB_CLIENT_RESULT iothubResult;

    // Use DAA cert when connecting - requires the SetDeviceId option to be set on the
    // IoT Hub client.
    static const int deviceIdForDaaCertUsage = 1;
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
             iothubClientHandle, "SetDeviceId", &deviceIdForDaaCertUsage)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"SetDeviceId\": %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
        goto cleanup;
    }

    // Sets auto URL encoding on IoT Hub Client
    bool urlAutoEncodeDecode = true;
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
             iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) !=
        IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set auto Url encode option on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
        goto cleanup;
    }

    // Sets model ID on IoT Hub Client
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_MODEL_ID,
                                                        azureSphereModelId)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set the Model ID on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
        goto cleanup;
    }

cleanup:
    iothub_security_deinit();

    return retVal;
}

/// <summary>
/// Check networking and DAA status before connection
/// </summary>
/// <returns>True if networking and DAA are ready; false otherwise.</returns>
static bool IsReadyToConnect(void)
{
    bool networkingReady = false;

    // Verifies networking on device
    if (Networking_IsNetworkingReady(&networkingReady) != 0) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!networkingReady) {
        Log_Debug("ERROR: IoT Hub connection - networking not ready.\n");
        return false;
    }

    // Verifies authentication is ready on device
    bool currentAppDeviceAuthReady = false;
    if (Application_IsDeviceAuthReady(&currentAppDeviceAuthReady) != 0) {
        Log_Debug("ERROR: Application_IsDeviceAuthReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!currentAppDeviceAuthReady) {
        Log_Debug("ERROR: IoT Hub connection - device auth not ready.\n");
        return false;
    }

    return true;
}
