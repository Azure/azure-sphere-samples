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
#include "connection_iot_edge.h"

#define MAX_ROOT_CA_CERT_CONTENT_SIZE (3 * 1024)
#define MAX_HOSTNAME_LENGTH (256)
#define MAX_MODELID_LENGTH (512)

static char hostname[MAX_HOSTNAME_LENGTH + 1];
static char iotEdgeRootCACertContent[MAX_ROOT_CA_CERT_CONTENT_SIZE +
                                     1]; // Add 1 to account for null terminator.
static char azureSphereModelId[MAX_MODELID_LENGTH + 1];

IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;

static const int deviceIdForDaaCertUsage = 1; // A constant used to direct the IoT SDK to use
                                              // the DAA cert under the hood.

static Connection_StatusCallbackType connectionStatusCallback = NULL;

static bool SetUpAzureIoTHubClientWithDaa(void);
static ExitCode ReadIoTEdgeCaCertContent(const char *certPath);
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

    Connection_IotEdge_Config *config = context;
    if (NULL == config) {
        Log_Debug("ERROR: IoT Edge connection context cannot be NULL.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (NULL == config->edgeDeviceHostname) {
        Log_Debug("ERROR: IoT Edge connection config must specify a hostname.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (NULL == config->iotEdgeCACertPath) {
        Log_Debug(
            "ERROR: IoT Edge connection config must specify a path to a root CA certificate.\n");
        return ExitCode_Validate_ConnectionConfig;
    }

    if (strnlen(config->edgeDeviceHostname, MAX_HOSTNAME_LENGTH) == MAX_HOSTNAME_LENGTH) {
        Log_Debug("ERROR: Specified IoT Edge device hostname exceeds maximum length '%d'.\n",
                  MAX_HOSTNAME_LENGTH);
        return ExitCode_Validate_Hostname;
    }

    strncpy(hostname, config->edgeDeviceHostname, MAX_HOSTNAME_LENGTH);

    return ReadIoTEdgeCaCertContent(config->iotEdgeCACertPath);
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
        Log_Debug("IoTHubDeviceClient_LL_CreateFromDeviceAuth returned NULL.\n");
        retVal = false;
        goto cleanup;
    }

    IOTHUB_CLIENT_RESULT iothubResult;
    // Enable DAA cert usage when x509 is invoked
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
             iothubClientHandle, "SetDeviceId", &deviceIdForDaaCertUsage)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"SetDeviceId\": %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
        goto cleanup;
    }

    // Provide the Azure IoT device client with the IoT Edge root
    // X509 CA certificate that was used to setup the IoT Edge runtime.
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_TRUSTED_CERT,
                                                        iotEdgeRootCACertContent)) !=
        IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"TrustedCerts\": %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
        goto cleanup;
    }

    // Set the auto URL Encoder (recommended for MQTT).
    bool urlEncodeOn = true;
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
             iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn)) !=
        IOTHUB_CLIENT_OK) {
        Log_Debug(
            "ERROR: Failure setting Azure IoT Hub client option "
            "\"OPTION_AUTO_URL_ENCODE_DECODE\": %s\n",
            IOTHUB_CLIENT_RESULTStrings(iothubResult));
        retVal = false;
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
        Log_Debug("ERROR: IoT Edge connection - networking not ready.\n");
        return false;
    }

    // Verifies authentication is ready on device
    bool currentAppDeviceAuthReady = false;
    if (Application_IsDeviceAuthReady(&currentAppDeviceAuthReady) != 0) {
        Log_Debug("ERROR: Application_IsDeviceAuthReady: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (!currentAppDeviceAuthReady) {
        Log_Debug("ERROR: IoT Edge connection - device auth not ready.\n");
        return false;
    }

    return true;
}

/// <summary>
///     Read the certificate file and provide a null terminated string containing the certificate.
///     The function logs an error and returns an error code if it cannot allocate enough memory to
///     hold the certificate content.
/// </summary>
/// <returns>ExitCode_Success on success, any other exit code on error</returns>
static ExitCode ReadIoTEdgeCaCertContent(const char *iotEdgeRootCAPath)
{
    int certFd = -1;
    off_t fileSize = 0;

    certFd = Storage_OpenFileInImagePackage(iotEdgeRootCAPath);
    if (certFd == -1) {
        Log_Debug("ERROR: Storage_OpenFileInImagePackage failed with error code: %d (%s).\n", errno,
                  strerror(errno));
        return ExitCode_IoTEdgeRootCa_Open_Failed;
    }

    // Get the file size.
    fileSize = lseek(certFd, 0, SEEK_END);
    if (fileSize == -1) {
        Log_Debug("ERROR: lseek SEEK_END: %d (%s)\n", errno, strerror(errno));
        close(certFd);
        return ExitCode_IoTEdgeRootCa_LSeek_Failed;
    }

    // Reset the pointer to start of the file.
    if (lseek(certFd, 0, SEEK_SET) < 0) {
        Log_Debug("ERROR: lseek SEEK_SET: %d (%s)\n", errno, strerror(errno));
        close(certFd);
        return ExitCode_IoTEdgeRootCa_LSeek_Failed;
    }

    if (fileSize == 0) {
        Log_Debug("File size invalid for %s\r\n", iotEdgeRootCAPath);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileSize_Invalid;
    }

    if (fileSize > MAX_ROOT_CA_CERT_CONTENT_SIZE) {
        Log_Debug("File size for %s is %lld bytes. Max file size supported is %d bytes.\r\n",
                  iotEdgeRootCAPath, fileSize, MAX_ROOT_CA_CERT_CONTENT_SIZE);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileSize_TooLarge;
    }

    // Copy the file into the buffer.
    ssize_t read_size = read(certFd, &iotEdgeRootCACertContent, (size_t)fileSize);
    if (read_size != (size_t)fileSize) {
        Log_Debug("Error reading file %s\r\n", iotEdgeRootCAPath);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileRead_Failed;
    }

    // Add the null terminator at the end.
    iotEdgeRootCACertContent[fileSize] = '\0';

    close(certFd);
    return ExitCode_Success;
}
