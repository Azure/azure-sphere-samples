# Run the sample with Azure IoT Central

**IMPORTANT**: Follow the instructions in the [README.md](./README.md) file before you perform any of the steps described here. Currently, these instructions are written for Windows and Visual Studio. 

To run the sample with Azure IoT Central, you must:

- Create an Azure IoT Central application
- Configure the Azure IoT Central Application
- Set up Azure IoT Central to work with your Azure Sphere tenant
- Configure the sample application to work with your Azure Sphere tenant and devices

You can then build and run the application. Later, you can enhance the Azure IoT Central application to support additional features. 

## Create an Azure IoT Central application

1. Go to [Azure IoT Central](https://apps.azureiotcentral.com/build) in your browser.

1. Choose Custom apps and sign in with your Microsoft account, or create a new account.

1. Use the default Application name and URL or modify it, if you prefer.

1. Under Application template, choose Custom Application.

1. Either choose 7 day free trial and provide your contact info, or provide your billing info.

1. Click **Create** at the bottom of the page.

## Configure the Azure IoT Central application

1. On the Home Page, select **Device Templates**. 

   ![Create Device Templates button](media/CreateDeviceTemplate.png)

1. On the **Select Template Types** page, select `Azure Sphere Sample Device` as the Preconfigured Device Template. Click **Customize** and then **Create**.

1. Now add two Views to the Device Template, one to send a command to the device, and one to show Temperature and Button Press. Select **Views**. 

1. To set up a view to send a command to the device, select **Editing device and cloud data**. Drag the **Status LED** property to the empty dotted rectangle to the right side of the form. Click **Save**. 

   ![Create Edit Device and Cloud Data View](media/FormView.png)

1. To set up a view of Temperature and Button telemetry, select **Views** and then **Visualizing the device**. Drag the **Button Press** and then **Temperature** telemetry to the empty space on the right side of the view. Click **Save**. 

1. Publish the Device Template so that it can be used in the Azure IoT Central application. Select **Publish** from the top menu bar.

## Set up Azure IoT Central to work with Azure Sphere

Verify the identity of your Azure Sphere tenant by following the steps in [set up Azure IoT Central](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-central#step-2-download-the-tenant-authentication-ca-certificate). Start at step 2 and perform steps 2-5. 

## Configure the sample application to work with your Azure Sphere tenant and devices

To configure the sample application, you'll need to supply the following information in the app_manifest.json file for AzureIoT:

- The Tenant ID for your Azure Sphere device
- The Scope ID for your Azure IoT Central application
- The allowed connections for your Azure Sphere device

Follow these steps to gather the information and configure the application:

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.
1. In Solution Explorer, find the app_manifest.json file and open it.
1. At the Azure Sphere Developer Command Prompt, issue the following command to get the tenant ID. Copy the returned GUID for your tenant and paste it into the **DeviceAuthentication** field of the app_manifest.json file:

   `azsphere tenant show-selected`
1. At the command prompt, run the ShowIoTCentralConfig program from the Windows or Linux folder in the sample repository. For example: For Windows, the path is Samples\AzureIoT\Tools\win-x64\ShowIoTCentralConfig.exe. When running this tool on a Linux machine you may need to explicitly set permissions. For example: From a terminal, run `chmod +x ShowIoTCentralConfig` to set permissions on the tool. 

   Now follow the prompts that the tool provides, and copy the information from the output into the app_manifest.json file in Visual Studio.

   * IoT Central App URL – this can be found in your browser address bar. For Example: http://myiotcentralapp.azureiotcentral.com/

   * API token – this can be generated from your IoT Central application. In the Azure IoT Central application select **Administration**, select **API Tokens**, select **Generate Token**, provide a name for the token (for example, "AzureSphereSample"), select **Administrator** as the role, and click **Generate**. Copy the token to the clipboard. The token starts with **SharedAccessSignature**.

   * ID Scope - In the Azure IoT Central application, select **Administration** > **Device Connection** and then copy the **ID Scope**

   **Note**: Your organization might require consent for the ShowIoTCentralConfig tool to access your Azure IoT Central data in the same way that the Azure API requires such consent. In some organizations, [enterprise application permissions](https://docs.microsoft.com/azure-sphere/install/admin-consent) must be granted by an IT administrator. 

1. Save the modified app_manifest.json file.
  
## Build and run the sample

1. Ensure that your device is connected to the internet.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an image package file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu on the tool bar, select **GDB Debugger (HLCore)**.

1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

   When the application starts, you should see output showing that the button and an LED have been opened, and that device authentication returned `AZURE_SPHERE_PROV_RESULT_OK`. The application then starts to send periodic messages with simulated temperatures to IoT Central.

1. Press button A. The Device Output display in Visual Studio shows the following message:

   ```
   Sending Azure IoT Hub telemetry: { "ButtonPress": "True" }
   INFO: IoTHubClient accepted the telemetry event for delivery
   ```

## Show your device data in Azure IoT Central

1. In your Azure IoT Central application, select **Devices** > **All Devices**. You should see your device listed as Unassociated and Unassigned (if you don’t see this then refresh the page).

1. Select your device and then select **Migrate**. Select the Azure Sphere Sample Device template, and click **Migrate**.

1. To view output from your Azure Sphere device, select **Devices** > **Azure Sphere Sample Device** and then select your device. You may change the device name at this point by selecting the name, modifying it, and selecting anywhere on the screen to save. 

1. Select **View**, which is on the menu bar just under the device name. Note that the device is sending simulated temperatures at regular intervals. Each time you press button A, an event is added to the button press graph. The graphs show data averaged over 30 seconds and not every individual event will be visible on the graphs. To see the count of button presses, hover the cursor over the chart. Note that you will not see previous data on the Button Press chart initially and will instead see "Waiting for data".

1. Turn on an LED on your Azure Sphere device from the Azure IoT Central application. Select the **Form** tab, click the **Status LED** checkbox, and select **Save**. In a few seconds, the LED lights up. 

1. Trigger an alarm from the Azure IoT Central application. Select the **Command** tab and click **Run**. Select the small history icon in the right corner of the Interface/Trigger Alarm box. This will show you the response to command from your sphere device. The Device Output display in Visual Studio shows the following message:

   ```
   ----- ALARM TRIGGERED! -----
   ```

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
