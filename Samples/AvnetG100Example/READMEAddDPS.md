# Add support to the sample for connecting via Azure IoT Hub device provisioning service

## Step 1. Start with the Azure IoT README topic

Follow the instructions in the [READMEStartWithHub.md](./READMEStartWithHub.md) file before you perform any of the steps described here.

## Step 2. Configure the device provisioning service

You must [set up the device provisioning service to provision your Azure Sphere devices into your IoT Hub(s)](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub?tabs=cliv2beta#authenticate-using-the-device-provisioning-service), if you have not already done so.

## Step 3. Update the sample application to connect via device provisioning service

By default, the application's top-level CMakeLists.txt pulls in code to connect directly to an IoT Hub. You'll need to modify this CMakeLists.txt to pull in the code for device provisioning service instead, and change some information in the app_manifest.json file:

1. Find the CMakeLists.txt file in your sample directory and open it.

1. Change the *add_subdirectory* CMake function call so that it no longer adds the code in the "IoTHub" directory and instead adds the code in the "DPS" subdirectory. The call should now look like this:

      `add_subdirectory(DPS)`

1. After making the changes, save the modified CMakeLists.txt file.

1. Log in to the [Azure Portal](https://portal.azure.com) and navigate to your device provisioning service. You will need to refer to service details while configuring the application.

1. Find the app_manifest.json file in your sample directory and open it. In the following steps, you'll add the following information to this file:

   - The Scope ID for your device provisioning service.
   - The allowed connections for your Azure Sphere device.

1. Update the *CmdArgs* field of the app_manifest.json file.

   - In the Azure portal, on the summary screen at the top right of your device provisioning service, copy the ID Scope value and add it to the *CmdArgs* field of the app_manifest.json file, removing any direct configuration of the Hub hostname, as shown below:

      `"CmdArgs": [ "--ScopeID", "<scope_id>" ]`

1. Update the *AllowedConnections* field of the app_manifest.json file.

   - Add "global.azure-devices-provisioning.net" to the list of connections in the *AllowedConnections* field.

   - In the Azure portal, under **Settings** of your device provisioning service, click **Linked IoT Hubs**. Copy the Name values for the Azure IoT Hub and append them to the *AllowedConnections* field of the app_manifest.json file, if they are not there already.

   - Each connection must be surrounded by double quotes. Separate connections with a comma.

   - Your *AllowedConnections* field should now look like:

     `"AllowedConnections": [ "global.azure-devices-provisioning.net", "<linked_iot_hub>" ]`

1. Save the modified app_manifest.json file.

## Step 4. Re-build and re-run the sample

Repeat the steps to [build and run the sample](./READMEStartWithIoTHub.md#Step-4.-build-and-run-the-sample).

## Troubleshooting

For common errors and corrective action, see [Troubleshooting](./troubleshooting.md).

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- See the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery#azure-sphere-gallery), a collection of unmaintained scripts, utilities, and functions.
