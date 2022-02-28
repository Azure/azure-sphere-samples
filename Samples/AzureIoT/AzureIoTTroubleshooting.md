# Troubleshooting Azure IoT sample apps

Troubleshooting content in this topic addresses scenarios in [Azure-sphere-samples/Samples/AzureIoT/Readme.md](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/README.md).

## All Azure IoT scenarios

1. The following message in the device output may indicate that device authentication is not ready:

   ```
   ERROR: <connection type> - device auth not ready.
   ```

   This error may occur if:

   - The tenant ID specified in the *DeviceAuthentication* field of your app_manifest.json file is incorrect. Verify that it matches the ID of the tenant your device is claimed into.

   - Your device is not claimed into a tenant. Go through the steps to [claim your device](https://docs.microsoft.com/azure-sphere/install/claim-device?tabs=cliv2beta).

## Azure Sphere device with a direct connection to your Azure IoT hub

1. The following messages in the device output may indicate a device registration or certificate authority error:

   ```
   Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
   Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
   ```
   These errors may occur if:

   - The device is not registered with the IoT Hub. Go through the steps to register device with Hub again. See Step 5 in [Set up an Azure IoT Hub to work with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#step-5.-create-an-x.509-device-in-your-iot-hub-for-your-azure-sphere-device).

   - Incorrect or no certificate is uploaded to the IoT hub. See Steps 2-4 in [Set up an Azure IoT hub to work with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub).

2. The following message in the device output may indicate a problem with the configuration of the Azure IoT hub in your app_manifest.json file:

   ```
   INFO: Azure IoT Hub connection started.
   INFO: Azure IoT Hub connection complete.
   Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
   ```

   This error may occur if:

   - The Azure IoT hub specified in the app_manifest.json doesn't exist. Verify that you've used the fully-qualified hub hostname; for example, contoso-hub-1234.azure-devices.net and not just contoso-hub-1234. Also verify that your hub has been created successfully and that it is deployed.

   - The Azure IoT hub hostname specified in *CmdArgs* does not match that specified in *AllowedConnections*, or vice-versa, or is missing from one of them. Verify that the Azure IoT hub hostname is correctly specified in both the *CmdArgs* and *AllowedConnections* fields of your app_manifest.json file.

## Azure IoT Central and device provisioning service

1. The following messages in the device output may indicate a problem in the configuration of your IoT Central application or device provisioning service instance:

   ```
   ERROR: Failed to register device with provisioning service: PROV_DEVICE_RESULT_DISABLED
   ```

   or

   ```
   ERROR: Failed to register device with provisioning service: PROV_DEVICE_RESULT_TRANSPORT
   ```

   These errors may occur if:

   - Device enrollment group has not been created or is disabled in the device provisioning service or in IoT Central. Go through the steps to [set up IoT Central](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-central?tabs=cliv2beta) or to [configure the device provisioning service](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub-with-dps?tabs=cliv2beta) again.

   - Incorrect certificate uploaded. Go through the steps to [set up IoT Central](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-central?tabs=cliv2beta) or to [configure device provisioning service](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub-with-dps?tabs=cliv2beta) again.

2. The following messages in the device output may indicate a misconfiguration of the Azure IoT Hub in your app_manifest.json file:

   ```
   INFO: Azure IoT Hub connection started.
   INFO: DPS device registration successful
   INFO: Azure IoT Hub connection complete.
   Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
   ```

   These errors may occur if:

   - The hub hostname is missing from, or incorrect in, the *AllowedConnections* field in your app_manifest.json file.
   - If you are using the device provisioning service, verify that you've used the fully-qualified hub hostname; for example, contoso-hub-1234.azure-devices.net and not just contoso-hub-1234.
   - Verify that the enrollment group is pointing to the correct hub in the Azure Portal. Review steps to [configure device provisioning service](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub-with-dps?tabs=cliv2beta).
   - If you are using IoT Central, re-run ShowIoTCentralConfig.exe and make sure you copy the *AllowedConnections* field correctly.

## Azure IoT Edge

1. The following messages in the device output may indicate a connection error with the IoT Edge device:

    ```
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    ```

   These errors may occur if:

   - Inbound port 8883 has not been opened on the IoT Edge device. For more information, see Step 3 in [Open IoT Edge gateway device ports for communication](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-edge?tabs=cliv2beta#step-3-open-iot-edge-gateway-device-ports-for-communication).
   - The sample is using an incorrect or invalid IoT Edge device root CA certificate.
   - The *edgeAgent* and *edgeHub* modules are not running on the IoT Edge device.

1. The following messages in the device output may indicate a device provisioning error:

    ```
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
    ```

   These errors may occur if:

   - The Azure Sphere device is not manually provisioned. Follow the steps to [authenticate using a direct connection](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub?tabs=cliv2beta).
