# Run the sample with an IoT Hub

**IMPORTANT**: Follow the instructions in the [README.md](./README.md) file before you perform any of the steps described here.

This sample can connect to the IoT Hub in two ways. It can connect directly to an IoT Hub with a device manually provisioned, or it can connect using the device provisioning service.

## Before you begin

Before you run the sample with an IoT Hub:

- Configure an IoT Hub to work with Azure Sphere based on how you want to connect to your IoT Hub. The device provisioning service is the recommended authentication method.
- Configure the sample application with information about your Azure Sphere tenant and your IoT Hub.

You can then build the application and, if you want to, use a device twin to support additional features. For more information about device twins, see [Understand and use device twins in IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins).

## Configure an IoT Hub

You must set up an IoT Hub for Azure Sphere, if you have not already done so.

To set up an IoT Hub where the sample application connects to the IoT Hub using the device provisioning service, [use device provisioning service to authenticate.](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#authenticate-using-the-device-provisioning-service) Note that the device provisioning service is the recommended authentication method.

To set up an IoT Hub where the sample application connects directly to the IoT Hub, [use a direct connection to authenticate.](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#authenticate-using-a-direct-connection)

## Configure the sample application to work with your Azure IoT Hub

To configure the sample application, you'll need to update some information in the app_manifest.json file for AzureIoT. The type of information depends on the connection type to the IoT Hub.

### Connect to the IoT Hub using the device provisioning service

To connect using the device provisioning service, you will need the following information:

- The Tenant ID for your Azure Sphere device.
- The Scope ID for your device provisioning service instance.
- The Azure IoT Hub URL for your IoT Hub and the global access link to device provisioning service (global.azure-devices.provisioning.net).
- The connection type to use when connecting to the IoT Hub.

Use the following steps to gather the information and configure the application:

1. Log in to the [Azure Portal](https://portal.azure.com) and navigate to your device provisioning service. You will need to refer to service details while configuring the application.

1. Find the app_manifest.json file in your sample directory and open it.

1. Update the **CmdArgs** field of the app_manifest.json file.
   - To configure the sample to use the device provisioning service to connect to the Azure IoT Hub, copy and paste the following line into the  **CmdArgs** field of the app_manifest.json file:

      `"--ConnectionType", "DPS"`

   - In the Azure portal, on the summary screen at the top right of your device provisioning service, copy the ID Scope value and append it to the **CmdArgs** field of the app_manifest.json file as shown below:

      `"--ScopeID", "<scope_id>"`

	  - You can also find the ID Scope value in Visual Studio Cloud Explorer. Expand **IoT Hub Device Provisioning Services** under **Azure Sphere Product Services** in Cloud Explorer, then select your device provisioning service. Click the **Properties** tab in the lower pane of Cloud Explorer and look for ID Scope in the list of properties.  
   
     - Each command line option must be surrounded by double quotes. Separate command line options with a comma.
	   
     - Your **CmdArgs** field should now look like:
   
       `"CmdArgs": [ "--ConnectionType", "DPS", "--ScopeID", "<scope_id>" ]`
1. Update the **AllowedConnections** field of the app_manifest.json file.

   - On the Azure portal, under **Settings** of your device provisioning service, select **Linked IoT Hubs**. Copy the Name values(s) for the Azure IoT Hub(s) and append them to the **AllowedConnections** field of the app_manifest.json file.

   - Make sure that global.azure-devices-provisioning.net remains in the list; this name is required for access to the device provisioning service.

   - Each connection must be surrounded by double quotes. Separate connections with a comma.

   - Your **AllowedConnections** field should now look like:

     `"AllowedConnections": [ "global.azure-devices-provisioning.net", "<linked_iot_hub>" ]`

1. Update the **DeviceAuthentication** field of the app_manifest.json file.

   - At the command prompt, use the following command to get the Tenant ID. Use the GUID, not the friendly name, and paste it into the **DeviceAuthentication** field of the app_manifest.json file:

      `azsphere tenant show-selected`

   - Your **DeviceAuthentication** field should now look like:

     `"DeviceAuthentication": "<GUID>"`

1. Save the modified app_manifest.json file.

### Connect to the IoT Hub directly

To configure a direct connection to IoT Hub, you will need the following information:

- The Tenant ID for your Azure Sphere device.
- The Device ID of your Azure Sphere device.
- The Azure IoT Hub server hostname for your IoT Hub.
- The connection type to use when connecting to the IoT Hub.

Follow these steps to gather the information and configure the application:

1. Log in to the Azure Portal and navigate to your Azure IoT Hub. You will need to refer to this later while configuring the application.

1. Find the app_manifest.json file in your sample directory and open it.

1. Update the **CmdArgs** field of the app_manifest.json file:
   - To configure the sample to connect directly to the IoT Hub, copy and paste the following line into the  **CmdArgs** field of the app_manifest.json file:

      `"--ConnectionType", "Direct"`

   - In the Azure portal, on the summary screen at the top right of your device provisioning service, copy the Hostname of Azure IoT Hub and paste it into the **CmdArgs** field of the app_manifest.json file as an argument to the Hostname option, as shown below:

      `"--Hostname", "<azure_iot_hub_hostname>"`

      - Each command line option must be surrounded by double quotes. Separate command line options with a comma.

      - Your **CmdArgs** field should now look like:

        `"CmdArgs": [ "--ConnectionType", "Direct", "--Hostname", "<azure_iot_hub_hostname>" ]`

1. Update the **AllowedConnections** field of the app_manifest.json file.

   - Copy the hostname used in the **CmdArgs** section and append it to the **AllowedConnections** field of the app_manifest.json file.

   - Each connection must be surrounded by double quotes. Separate connections with a comma.

   - Your **AllowedConnections** field should now look like:

     `"AllowedConnections": [ "<azure_iot_hub_hostname>" ]`

1. Update the **DeviceAuthentication** field of the app_manifest.json file.

   - At the command prompt, run the following command to get the Tenant ID. Use the GUID, not the friendly name, and paste it into the **DeviceAuthentication** field of the app_manifest.json file:

      `azsphere tenant show-selected`

   - Your **DeviceAuthentication** field should now look like:

     `"DeviceAuthentication": "<GUID>"`

1. Save the modified app_manifest.json file.

## Build and run the sample

1. Ensure that your device is connected to the internet.

1. Follow the steps in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

1. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

1. After a short delay, the sample app will display output messages like:

   **INFO: Azure IoT Hub send telemetry: { "Temperature": "33.85" }**

1. Press button A on the MT3620 development board to send a button-press notification to the IoT Hub. The sample app will display an output message indicating a button-press:

    **Sending Azure IoT Hub telemetry: { "ButtonPress": "True" }**

## Edit device twin to change properties

You can now edit the device twin to change properties. For example, follow these steps to turn LED 1 on or off by changing a property in the device twin:

1. Log in to the [Azure Portal](https://portal.azure.com) and select your IoT Hub resource.

1. On the left-side menu under **Explorers**, select **IoT Devices**, and then double-click the Device ID for your device.

1. On the **Device Details** page, select **Device Twin**.

1. In the **properties** field, under **"desired"**, add `"StatusLED": true,` as shown below:

   ```json
      "properties": {
         "desired": {
            "StatusLED": true,
            "$metadata": {
            "$lastUpdated": "2019-01-30T22:18:19.612025Z",
   ```

1. Click **Save** to update the twin and notify the application.
In a few seconds, the LED will light up red.

## Troubleshooting

1. The following message in device output indicates a connection error:

   `IOTHUB_CLIENT_NO_CONNECTION`

   This error may occur if:

   - The **AllowedConnections** field has not been properly updated in the application manifest .json file.

   The application may generate output like "IoTHubClient accepted the message for delivery" while telemetry is generated. This indicates that the IoTHubClient has accepted the message for delivery but the data has not yet been sent to the IoT hub.

1. The following message in device output indicates an authentication error:

   'IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned 'AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY'.'

   This error may occur if:

   - The correct Tenant ID is not present in the **DeviceAuthentication** field of the application manifest.
   - The device has not been claimed.

1. The following message in device output indicates a device provisioning error:

   'IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned 'AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR'.'

   This error may occur if:

   - [Azure IoT Hub](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub) setup has not been completed.
