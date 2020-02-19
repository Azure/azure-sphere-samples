# Run the sample with an IoT hub

**IMPORTANT**: Follow the instructions in the [README.md](./README.md) file before you perform any of the steps described here.

To run the sample with an IoT hub you must first:

- Configure an IoT hub to work with Azure Sphere
- Configure the sample application with information about your Azure Sphere tenant and your IoT hub

You can then build the application and, if you want, use a device twin to support additional features.

## Configure an IoT hub

[Set up an IoT hub for Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub), if you have not already done so.

## Configure the sample application to work with your Azure IoT Hub

To configure the sample application, you'll need to supply the following information in the app_manifest.json file for AzureIoT:

- The Tenant ID for your Azure Sphere device
- The Scope ID for your device provisioning service (DPS) instance
- The Azure IoT Hub URL for your IoT hub, along with the global access link to DPS (global.azure-devices.provisioning.net)

Follow these steps to gather the information and configure the application:

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file app_manifest.json and then click **Open**.
1. At the command prompt, use the following command to get the tenant ID. Copy the returned value and paste it into the **DeviceAuthentication** field of the app_manifest.json file:

   `azsphere tenant show-selected`
1. Log in to [Azure Portal](https://portal.azure.com) and navigate to your DPS.
1. In the summary screen at the top right, copy the ID Scope value and paste it into the **CmdArgs** field of the app_manifest.json file.
1. Under Settings, select Linked IoT Hubs. Copy the Service endpoint values(s) for the Azure IoT Hub(s) and append them to the **AllowedConnections** field of the app_manifest.json file. Make sure that global.azure-devices.provisioning.net remains in the list; this name is required for access to DPS. Each connection should be identified with double quotes and separated by a comma.

1. Save the modified app_manifest.json file.

## Build and run the sample

1. Ensure that your device is connected to the internet.
1. In Visual Studio, open **View > Cloud Explorer**. [Navigate to your Azure subscription](https://docs.microsoft.com/visualstudio/azure/vs-azure-tools-resources-managing-with-cloud-explorer) and select your IoT hub resource.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

1. On the **Actions** tab of Cloud Explorer, select **Start Monitoring D2C Message**.
1. In the Visual Studio Output window, navigate to Show output from: IoT Hub. This window displays information about each device-to-cloud (D2C) message that the IoT hub receives.
1. After a short delay, the IoT hub output should display messages that begin with text like this and continue for several lines:

   `[Monitor D2C Message] [1/30/2019 2:36:33 PM] Message received on partition 0:{ "Temperature": "33.85" }`
1. Press button A on the MT3620 development board to send a button-press notification to the IoT hub. The IoT hub output displays a message indicating a button-press:

    `Sending IoT Hub Message: { "ButtonPress": "True" }`
1. Press button B on the MT3620 development board to send a simulated device orientation to the IoT hub. The IoT hub output displays a message indicating a message containing the simulated device orientation:
 
   `Sending IoT Hub Message: { "Orientation": "Up" }`

## Edit device twin to change properties

You can now edit the device twin to change properties. For example, follow these steps to turn LED 1 on or off by changing a property in the device twin:

1. Log in to [Azure Portal](https://portal.azure.com) and select your IoT hub resource.
1. On the left-side menu under Explorers, select IoT Devices, and then double-click the device ID for your device.
1. On the Device Details page, select Device Twin. 
1. In the **properties** field, under **"desired"**, add `"StatusLED": { "value": true},` as shown in this excerpt:

```json
   "properties": {
       "desired": {
         "StatusLED": {
            "value": true
         },
         "$metadata": {
           "$lastUpdated": "2019-01-30T22:18:19.612025Z",
```

1. Click **Save** to update the twin and notify the application.
In a few seconds, the LED lights up red.

## Troubleshooting

The following message in the Visual Studio Device Output indicates an authentication error:

   `IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned 'AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY'.'`

This error may occur if:

- The correct tenant ID is not present in the **DeviceAuthentication** field of the application manifest
- The device has not been claimed

The following message in the Visual Studio Device Output indicates a device provisioning error:

   `IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned 'AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR'.'`

This error may occur if:

- The [setup for Azure IoT Central](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-central) or [Azure IoT Hub](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub) has not been completed
