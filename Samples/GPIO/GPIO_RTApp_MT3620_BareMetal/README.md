# Sample: MT3620 real-time capability application - Bare Metal GPIO

This sample demonstrates how to use GPIO on an MT3620 real-time core. It performs the same function as the high-level [GPIO Sample application](../GPIO_HighLevelApp/README.md).

- It provides access to one of the LEDs on the MT3620 development board using GPIO.
- It uses a button to change the blink rate of the LED.

However, it runs directly on one of the real-time cores instead of the high-level core. See [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview#real-time-capable-applications) to learn about the differences between high-level and real-time capable applications (RTApps) and to find links to additional information about RTApps.

The sample uses a general-purpose timer (GPT) on the real-time core to control the LED blink rate. For more information about timers, see [General-purpose timers](https://docs.microsoft.com/azure-sphere/app-development/use-peripherals-rt#general-purpose-timers).

To use this sample, clone the repository locally if you haven't already done so:

```
git clone https://github.com/Azure/azure-sphere-samples.git
```
  
## Prerequisites

- [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

## To build and run the sample

**Prep your device**

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.05 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More > Run as administrator**.
1. At the command prompt issue the following command:

   ```
   azsphere dev prep-debug --EnableRTCoreDebugging
   ```

   This command must be run as administrator when you enable real-time core debugging because it installs USB drivers for the debugger.
1. Close the window after the command completes because administrator privilege is no longer required.  
    **Note:** As a best practice, you should always use the lowest privilege that can accomplish a task.

**Build and deploy the application**

1. Start Visual Studio.
1. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This step automatically performs the manual packaging steps. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (RTCore)**.
1. Press F5 to start the application with debugging. LED1 will blink red. Press button A to change the blink rate.

## License

For details on license, see LICENSE.txt in this directory.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).