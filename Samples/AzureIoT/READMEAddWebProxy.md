# Add proxy support to the sample

This sample shows how to add proxy support to the AzureIoT sample, either by connecting to an Azure IoT Hub directly, or through the Azure IoT Hub device provisioning service.

The Azure Sphere 22.02 release includes proxy support. For details, see [Connect Azure Sphere through a proxy server](https://docs.microsoft.com/azure-sphere/network/connect-through-a-proxy).

## Step 1. Retrieve the proxy configuration

If you have previously configured proxy settings, you can retrieve them and validate that they are configured correctly. You can use a code snippet from [GetProxySettings](https://github.com/Azure/azure-sphere-samples/tree/main/CodeSnippets/Networking/Proxy/GetProxySettings) to retrieve the proxy configuration.

If you confirm that your proxy settings are correctly configured, you can skip the next step for configuring the proxy settings.

## Step 2. Configure the proxy settings

If you have not previously configured proxy settings, configure the application to connect through a proxy server (this assumes that you have a proxy server configured on your network).

In the AzureIoT sample main function, [`main.c`](../Common/main.c), add a call to configure the proxy settings for the application. You can use a code snippet from [SetProxySettings](https://github.com/Azure/azure-sphere-samples/tree/main/CodeSnippets/Networking/Proxy/SetProxySettings) to configure proxy settings.

Configuring the application proxy settings ensures that calls to [Networking_IsNetworkingReady](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/function-networking-isnetworkingready) are successful.

## Step 3. Update the sample

The AzureIoT sample application can be configured to communicate with an IoT Hub directly, or to use the Azure Device Provisioning Services (DPS).

### Connect to AzureIoT hub directly

You can use the proxy sample code in the [READMEStartWithIoTHub.md](./READMEStartWithIoTHub.md) sample and make the following changes:

In [`connection_iot_hub.c`](./IoTHub/connection_iot_hub.c) add an include as follows:

```cpp
#include <iothubtransportmqtt_websockets.h>
```

In the function `SetUpAzureIoTHubClientWithDaa` find the call to `IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth` and replace it with the following:

```cpp
    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(hostname, MQTT_WebSocket_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: IoTHubDeviceClient_LL_CreateFromDeviceAuth returned NULL.\n");
        retVal = false;
        goto cleanup;
    }

    IOTHUB_CLIENT_RESULT iothubResult;

    HTTP_PROXY_OPTIONS httpProxyOptions;
    memset(&httpProxyOptions, 0, sizeof(httpProxyOptions));
    httpProxyOptions.host_address = PROXY_ADDRESS;
    httpProxyOptions.port = PROXY_PORT;

    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(
        iothubClientHandle, OPTION_HTTP_PROXY, &httpProxyOptions)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option  \"OPTION_HTTP_PROXY\": %s\n",
            IOTHUB_CLIENT_RESULTStrings(iothubResult));
            retVal = false;
            goto cleanup;
    }
```

### Connect via Azure IoT Hub device provisioning service

You can use the proxy sample code in the [ReadmeAddDPS](ReadmeAddDPS.md) or [ReadmeStartWithIoTCentral](ReadmeStartWithIoTCentral.md) sample and make the following changes:

In [`connection_dps.c`](./DPS/connection_dps.c), add includes as follows:

```cpp
#include <iothubtransportmqtt_websockets.h>
#include <azure_prov_client/prov_transport_mqtt_ws_client.h>
```

In the function `InitializeProvisioningClient` find the call to `Prov_Device_LL_Create` and replace it with the following:

```cpp
    // Create Provisioning Client for communication with DPS
    // using MQTT protocol
    provHandle = Prov_Device_LL_Create(dpsUrl, scopeId, Prov_Device_MQTT_WS_Protocol);
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

    HTTP_PROXY_OPTIONS httpProxyOptions;
    memset(&httpProxyOptions, 0, sizeof(httpProxyOptions));
    httpProxyOptions.host_address = PROXY_ADDRESS;
    httpProxyOptions.port = PROXY_PORT;
    prov_result = Prov_Device_LL_SetOption(provHandle, OPTION_HTTP_PROXY, &httpProxyOptions);
    if (prov_result != PROV_DEVICE_RESULT_OK) {
        Log_Debug("ERROR: Failed to set Proxy options in Provisioning Client\n");
        goto cleanup;
    }
```

Then in the function `OnRegisterComplete` find the call to `IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth` and replace it with the following:

```cpp
    // Create Azure Iot Hub client handle
    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(iotHubUri, MQTT_WebSocket_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: Failed to create client IoT Hub Client Handle\n");
        goto cleanup;
    }

    IOTHUB_CLIENT_RESULT iothubResult;

    HTTP_PROXY_OPTIONS httpProxyOptions;
    memset(&httpProxyOptions, 0, sizeof(httpProxyOptions));
    httpProxyOptions.host_address = PROXY_ADDRESS;
    httpProxyOptions.port = PROXY_PORT;

    iothubResult = IoTHubDeviceClient_LL_SetOption(
        iothubClientHandle, OPTION_HTTP_PROXY, &httpProxyOptions);
    if (iothubResult != IOTHUB_CLIENT_OK) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
        iothubClientHandle = NULL;
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option  \"OPTION_HTTP_PROXY\": %s\n",
            IOTHUB_CLIENT_RESULTStrings(iothubResult));
        goto cleanup;
    }

    // Use DAA cert when connecting - requires the SetDeviceId option to be set on the
    // IoT Hub client.
    static const int deviceIdForDaaCertUsage = 1;
    iothubResult = IoTHubDeviceClient_LL_SetOption(
        iothubClientHandle, "SetDeviceId", &deviceIdForDaaCertUsage);
    if (iothubResult != IOTHUB_CLIENT_OK) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
        iothubClientHandle = NULL;
        Log_Debug("ERROR: Failed to set Device ID on IoT Hub Client: %s\n",
                  IOTHUB_CLIENT_RESULTStrings(iothubResult));
        goto cleanup;
    }
```
