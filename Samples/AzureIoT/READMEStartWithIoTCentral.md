# Run the sample by connecting to an Azure IoT Central application

## Step 1. Start with the Azure IoT README topic

Follow the instructions in the [README](./README.md) topic before you perform any of the steps described here.

**Note**: This sample uses the latest IoT Central message syntax and will not work with IoT Central applications created on or before July 7th, 2020.

## Step 2. Configure Azure IoT Central to work with Azure Sphere

You must [set up Azure IoT Central to work with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-central), if you have not already done so.

## Step 3. Enable the Azure IoT Central auto-approve option

1. Go to [Azure IoT Central](https://apps.azureiotcentral.com/myapps) in your browser.

1. Open your IoT Central application. Under the **Security** section, select **Permissions**, then **Device connection groups**. Set **Auto approve new devices** to **On**.

## Step 4. Modify the sample app_manifest.json file to connect to your IoT Central application

An Azure IoT Central application includes underlying Azure IoT Hub and a device provisioning service to provision devices into these hubs. By default, this application's top-level CMakeLists.txt pulls in code to connect directly to an IoT Hub. You'll need to modify this CMakeLists.txt to pull in the code to connect via DPS instead and change some information in the app_manifest.json file:

1. Find the CMakeLists.txt file in your sample directory and open it.

1. Change the *add_subdirectory* CMake function call so that it no longer adds the code in the IoTHub directory and instead adds the code in the DPS subdirectory. The call should now look like this:

   `add_subdirectory(DPS)`

1. Save the modified CMakeLists.txt file.

1. Find the app_manifest.json file in your sample directory and open it. In the following steps, you'll gather the following information and add it to this file:

   - The Scope ID for your Azure IoT Central application.
   - The allowed connections for your Azure Sphere device.
   - The Tenant ID for your Azure Sphere device.

1. At the command prompt, run the **ShowIoTCentralConfig** program from the Windows or Linux folder in the sample repository. For example, on Windows, the path is ..\Samples\AzureIoT\Tools\win-x64\ShowIoTCentralConfig.exe. When running this tool on a Linux machine you may have to explicitly set permissions. For example, from a terminal, run `chmod +x ShowIoTCentralConfig` to set permissions on the tool.

   **Note**: Do not run the **ShowIoTCentralConfig** program directly from the Start menu as the window will disappear. Launch a command prompt and run **ShowIoTCentralConfig** from there.

   **Note**: If you are unable to run the **ShowIoTCentralConfig** program at the command prompt, delete the %localappdata%\Temp\\.net\ShowIoTCentralConfig folder, then run the program again.

   Now follow the prompts that the tool provides, and copy the information from the output into the app_manifest.json file. The tool will require the following input:

   - The IoT Central App URL can be found in your browser address bar; for example, `https://myiotcentralapp.azureiotcentral.com/`.

   - The API token can be generated from your IoT Central application. In the Azure IoT Central application, select **Permissions** > **API Tokens** > **+ New**, then provide a name for the token; for example, **AzureSphereSample**. In **Role**, select **AppAdministrator**. Select **Generate**, then copy the token to the clipboard. The token starts with `SharedAccessSignature`.

   - The ID Scope is in the Azure IoT Central application. Select **Permissions** > **Device connection groups**, then copy the **ID Scope**.

1. At the command prompt, run the following command to get the Tenant ID:

   `azsphere tenant show-selected`

   Paste the GUID for your tenant into the **DeviceAuthentication** field of the app_manifest.json file.

1. Check that your updated app_manifest.json file has lines that looks like this:

      `"CmdArgs": [ "--ScopeID", "<scope_id>" ]`

   and:

      `"AllowedConnections": [ "global.azure-devices-provisioning.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net","iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net","iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net", "iotc-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.azure-devices.net" ],`

   and:

      `"DeviceAuthentication": "<GUID>"`

1. Save the updated application manifest.

## Step 5. Build and run the sample

1. Ensure that your device is connected to the internet.

1. Follow the steps in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

1. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

1. When the app is running, press button A on the MT3620 development board to change the Thermometer Telemetry Upload Enabled status to true. The app will start sending (simulated) temperature telemetry.  An LED on the device will light to indicate this status change, and the app will output messages like:

   **INFO: Sending Azure IoT Hub telemetry: {"Temperature":49.700000762939453}.**

1. Press button A again to change the Thermometer Telemetry Upload Enabled status back to false. The app will stop sending temperature telemetry, and the LED will turn off again.

1. Press button B to send a telemetry event indicating the thermometer has been moved to a new location. The app will output messages like:

   **INFO: Device Moved.**

   **Sending Azure IoT Hub telemetry: {"thermometerMoved":true}.**

## Step 6. View and edit the device data in Azure IoT Central

1. In your Azure IoT Central application, select your device. Select **Devices** > **All Devices**. You should see your device listed as an "Azure Sphere Example Thermometer". The device is identified in this way because it sends its [Azure IoT Plug and Play (PnP)](https://docs.microsoft.com/azure/iot-pnp/) model ID when it connects     .

1. Select your device, and then select **About** on the menu bar just under the device name. Note that the device has sent its serial number.

1. Select **Device templates** and select the **Azure Sphere Example Thermometer** template.

1. Select **Views** > **Editing device and cloud data**. Enter *Enable telemetry* as the Form name value. Select **Thermometer Telemetry Upload Enabled**. Select **Add section** > **Save** > **Publish**.

1. Select **Devices** > **All Devices**, then select your device and select **Enable telemetry**.

1. Select the dropdown menu under **Thermometer Telemetry Upload Enabled**, then select **True** > **Save**. In a few seconds, the LED will light up and the app will start sending telemetry.

1. Select **Overview**. See that the device is sending simulated temperatures at regular intervals. See also that the **Raw Data** will include any Thermometer Moved events when you press button B.

1. Run a command from the Azure IoT Central application. Select the **Command** tab.  In the Alert Message field, enter an example alert message like "Sunny later today. Close your window blinds to lower temperature!".  Select **Run**. Select the History icon in the right corner of the Interface/Trigger Alarm box. This will show you the response to the command from your Azure Sphere device. The device itself will also output the following message:

   ```
   ALERT: Sunny later today. Close your window blinds to lower temperature!
   ```

## Troubleshooting

For common errors and corrective action, see [Troubleshooting](./AzureIoTTroubleshooting.md).
