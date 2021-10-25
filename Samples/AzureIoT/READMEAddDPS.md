# Add support to the sample for connecting via Azure IoT Hub Device Provisioning Service

## Step 1. Start with the Azure IoT README topic

Follow the instructions in the [READMEStartWithIoTHub.md](./READMEStartWithIoTHub.md) file before you perform any of the steps described here.

## Step 2. Configure the Device Provisioning Service

Set up the Device Provisioning Service to provision your Azure Sphere devices into your IoT Hubs, if you have not already done so. For instructions on how to do this, see [Set up an Azure IoT Hub for Azure Sphere with the Device Provisioning Service](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub-with-dps).

## Step 3. Update the sample application to connect via Device Provisioning Service

By default, the application's top-level CMakeLists.txt file pulls in code to connect directly to an IoT Hub. You need to modify CMakeLists.txt to pull in the code for Device Provisioning Service instead, and change information in the app_manifest.json file:

1. Open the CMakeLists.txt file.

1. Change the *add_subdirectory* CMake function call so that it does not add the code in the IoTHub directory but instead adds the code in the DPS subdirectory. The call should now look like this:

      `add_subdirectory(DPS)`

1. Save the modified CMakeLists.txt file.

1. Log in to the [Azure Portal](https://portal.azure.com) and navigate to your Device Provisioning Service. You need to refer to service details while configuring the application.

1. Find the app_manifest.json file in your sample directory and open it. In the following steps, you'll add the following information to this file:

   - The Scope ID for your Device Provisioning Service.
   - The allowed connections for your Azure Sphere device.

1. Update the *CmdArgs* field of the app_manifest.json file.

   - In the Azure portal, on the summary screen at the top right of your Device Provisioning Service, copy the **ID Scope** value and add it to the *CmdArgs* field of the app_manifest.json file, removing any direct configuration of the Hub hostname, as shown below:

      `"CmdArgs": [ "--ScopeID", "<scope_id>" ]`

1. Update the *AllowedConnections* field of the app_manifest.json file.

   - Add `global.azure-devices-provisioning.net` to the list of connections in the *AllowedConnections* field.

   - In the Azure portal, under **Settings** for your Device Provisioning Service, select **Linked IoT Hubs**. Copy the IoT hub names from **IoT hub** and append them to the *AllowedConnections* field of the app_manifest.json file, if they are not there already.

   - Surround each connection with double quotation marks. Separate connections with commas.

   - Your *AllowedConnections* field should now look like this:

     `"AllowedConnections": [ "global.azure-devices-provisioning.net", "<linked_iot_hub>" ]`

1. Save the modified app_manifest.json file.

## Step 4. Re-build and re-run the sample

Repeat the steps to [build and run the sample](./READMEStartWithIoTHub.md#Step-4.-build-and-run-the-sample).

## Troubleshooting

For common errors and corrective action, see [Troubleshooting](./AzureIoTTroubleshooting.md).

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- See the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery#azure-sphere-gallery), a collection of unmaintained scripts, utilities, and functions.
