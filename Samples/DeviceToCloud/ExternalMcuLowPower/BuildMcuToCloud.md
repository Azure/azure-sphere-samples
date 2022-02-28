# Build, run, and deploy the External MCU, Low Power reference solution

Perform the following steps to build, run, and deploy this sample:

1. [Build and deploy the MCU application](#build-and-deploy-the-mcu-app).
1. [Create an IoT Central application and add views](#create-an-iot-central-application-and-add-views).
1. [Build and deploy the Azure Sphere MT3620 high-level application](#build-and-deploy-the-azure-sphere-mt3620-high-level-app).
1. [Run the solution](#run-the-solution).

## Build and deploy the MCU app

If you haven't done so already:

- Download and install the [STM32Cube IDE](https://www.st.com/en/development-tools/stm32cubeide.html#get-software).

- Connect the Azure Sphere development board and the NUCLEO-L031K6 developer board to your computer via USB.

To build and run the MCU app do the following:

1. Open the STM32Cube IDE.
1. In the **File menu** select **Open Projects from File System**.
1. In the **Import Projects from File System or Archive** window, click **Directory** and navigate to the *McuSoda* folder. Click **Select Folder**.
1. Click **Finish**.
1. In the **Project** menu select **Build All**
1. In the **Run** menu select **Run** or **Debug**.

    **Note:**  The first time you run or debug, a configuration menu will pop up. Select the defaults.

## Create an IoT Central application and add views

To create an IoT Central application and add views, see [IOT Central Setup](./IOTCentralSetup.md).

## Build and deploy the Azure Sphere MT3620 high-level app

To build and run the high-level app, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

**Note:**
 When the MT3620 is in Power Down state, it might be unresponsive to CLI commands or attempts to deploy a new or updated image from Visual Studio and Visual Studio Code. You may therefore need to manually restart the MT3620 during the interval when the device is not in Power Down state, using either the Reset button or the "azsphere device restart" CLI command.
 When running this sample, the status LED indicates when the MT3620 device is not in Power Down state.
 If this interval is too short, try the following:

   1. Use the Azure Sphere `azsphere device restart`  CLI command to restart the device.
   2. After the device has restarted, use the `azsphere device sideload delete` command to delete the sideloaded application that is causing Power Down state.

## Run the solution

**MCU Interactions:**

On startup the external MCU turns on and waits for the Azure Sphere MT3620 to send it a flavor and color. When the External MCU receives the flavor color from the MT3620, it will turn on the soda dispense color. Each time a new flavor is sent, the external MCU will update the dispense flavor color.

Every 2 minutes, the MT3620 turns on the status LED, wakes up the external MCU, collects data, and sends the data to IoT Central.

**IoT Central interactions:**

1. On startup, the MT3620 connects to IoT Central.
1. Go to IoT Central and select "devices" and migrate the new device that appears to the Soda Machine Device Template.
1. Select the device and then select "Next Flavor".
1. In the flavor box, provide the new flavor name (such as "Cherry") and set the color with the drop down then select save.
1. In the debug output from the MT3620, you will see the device receive the new flavor and pass it to the external MCU which will light up with the new flavor color.

**User interactions:**

- Press the **Dispense** button (connected to pin D9 on the NUCLEO-L031K6 board) to dispense one unit of the soda. The external MCU RGB LED will flash for three seconds with the current flavor.

- Press the **Refill** button (connected to pin D10 on the NUCLEO-L031K6 board) to refill the soda machine syrups and carbonated water.

- Use the 10K potentiometer to vary the input voltage to the external MCU ADC input (pin A3). This is used to simulate the battery voltage level, which is displayed in a view on  IoT Central.
  Note that the [hardware reference solution](https://github.com/Azure/azure-sphere-hardware-designs/tree/main/P-MT3620EXMSTLP-1-0) makes available a feature that enables you to monitor actual battery voltage levels.

## Build your own solution

To build your own solution, modify the `configuration.h` file located at `../LocalSamples/DeviceToCloud/ExternalMcuLowPower/common/configuration.h`. Open the file and change the value of the corresponding constant.

| Setting  | Default value | Description  |
| -------- |----------------| -------------|
| MachineCapacity           | 5 | The capacity (maximum units) of the soda machine |
| LowDispenseAlertThreshold | 2 | The number of remaining units that will initiate a low stock alert |
