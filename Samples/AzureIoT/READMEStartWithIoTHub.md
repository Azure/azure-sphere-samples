# Run the sample by connecting to an Azure IoT Hub

## Step 1. Start with the Azure IoT Readme topic

Follow the instructions in the [README](./README.md) topic before you perform any of the steps described here.

## Step 2. Configure an IoT Hub to work with Azure Sphere

[Set up an Azure IoT Hub for Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub) if you have not already done so.

## Step 3. Modify the sample app_manifest.json file to connect to your Azure IoT Hub

To configure the sample application, you'll update some information in the app_manifest.json file. Follow these steps to gather the information and configure the application:

1. Log in to the [Azure Portal](https://portal.azure.com) and navigate to your Azure IoT Hub. You will need to refer to this later while configuring the application.

1. Find the app_manifest.json file in your sample directory and open it. In the following steps, you'll add the following information to this file:

   - The hostname of your Azure IoT Hub.
   - The allowed connections for your Azure Sphere device.
   - The Tenant ID for your Azure Sphere device.

1. Update the *CmdArgs* field of the app_manifest.json file:
   - In the Azure portal, at the top right of your Hub's Overview screen, copy the Hostname and paste it into the *CmdArgs* field of the app_manifest.json file as an argument to the Hostname option. Your *CmdArgs* field should now look like:

        `"CmdArgs": [ "--Hostname", "<azure_iot_hub_name.azure-devices.net>" ]`

1. Update the *AllowedConnections* field of the app_manifest.json file.

   - Copy the hostname used in the *CmdArgs* section and append it to the *AllowedConnections* field of the app_manifest.json file. The field should now look like:

     `"AllowedConnections": [ "<azure_iot_hub_name.azure-devices.net>" ]`

1. Update the *DeviceAuthentication* field of the app_manifest.json file.

   - At the command prompt, run the following command to get the Tenant ID. Use the GUID, not the friendly name, and paste it into the *DeviceAuthentication* field of the app_manifest.json file:

      `azsphere tenant show-selected`

   - Your *DeviceAuthentication* field should now look like:

     `"DeviceAuthentication": "<GUID>"`

1. Save the modified app_manifest.json file.

## Step 4. Build and run the sample

1. Ensure that your device is connected to the internet.

1. Follow the steps in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

1. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

1. When the app is running, press button A on the MT3620 development board to change the Thermometer Telemetry Upload Enabled status to true. The app will start sending (simulated) temperature telemetry.  An LED on the device will light to indicate this status change, and the app will output messages like:

   **INFO: Sending Azure IoT Hub telemetry: {"Temperature":49.700000762939453}.**

1. Press button A again to change the Thermometer Telemetry Upload Enabled status back to false. The app will stop sending temperature telemetry, and the LED will turn off again.

1. Press button B to send a telemetry event indicating the thermometer has been moved to a new location. The app will output messages like:

   **INFO: Device moved**

## Step 5. View and edit the device data in IoT Hub

View device in IoT Hub:

1. Log in to the [Azure Portal](https://portal.azure.com) and select your IoT Hub resource.

1. On the left-side menu, select **Devices**, then select the Device ID for your device.

View and edit the device twin:

1. Select the **Device Twin** tab.

1. In the *properties* field, under **reported**, note that the device has reported its serial number.

1. In the *properties* field, under **desired**, add `"thermometerTelemetryUploadEnabled": true,` as shown below:

   ```json
      "properties": {
         "desired": {
            "thermometerTelemetryUploadEnabled": true,
            "$metadata": {
            "$lastUpdated": "2019-01-30T22:18:19.612025Z",
   ```

1. Click **Save** to update the twin and notify the application. In a few seconds, the LED will light up and the app will start sending telemetry.

Execute a direct method:

1. Click the **Direct Method** tab.

1. In the *Method Name* field, enter "displayAlert".

1. In the *Payload* field, enter an example alert message like "Sunny later today. Close your window blinds to lower temperature!".

1. Click **Invoke Method** to execute the method on the application. In a few seconds, the device will output the following:

   ```
   ALERT: Sunny later today. Close your window blinds to lower temperature!
   ```

You can view telemetry:

1. Start the Azure CLI in Cloud Shell. There is a button to launch this shell at the top of the portal.

1. Install the Azure IoT Hub extension:

   `az extension add --name azure-iot`

1. Run the following command:

   `az iot hub monitor-events --hub-name <azure_iot_hub_name> --output table`

1. Observe the telemetry arriving, both the regular temperature readings and thermometer-moved events when you press button B.

## Step 6. Note how the app uses Azure IoT PnP

The application implements an example [Azure IoT Plug and Play (PnP)](https://docs.microsoft.com/azure/iot-develop/overview-iot-plug-and-play) model called "Azure Sphere Example Thermometer".  It sends this model's ID when it connects to Azure IoT Hub.

Follow the [IoT Explorer instructions](https://docs.microsoft.com/azure/iot-pnp/howto-use-iot-explorer) to interact and test this feature.

Note how the Azure IoT Explorer client finds and uses the PnP model to understand the device's data, and you don't need to type the model's parameter names, like thermometerTelemetryUploadEnabled or displayAlert.

## Next steps

- [Connect via Azure IoT Hub device provisioning service](./READMEAddDPS.md)
- [Connect via Azure IoT Edge](./READMEAddIoTEdge.md)

## Troubleshooting

For common errors and corrective action, see [Troubleshooting](./AzureIoTTroubleshooting.md).
