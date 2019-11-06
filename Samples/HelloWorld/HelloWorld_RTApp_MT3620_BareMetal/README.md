# Sample: MT3620 real-time capability application - Hello World

This sample demonstrates how to create, deploy, and run [real-time capable applications on the MT3620 real-time cores](https://docs.microsoft.com/azure-sphere/quickstarts/qs-real-time-application).

This sample app for an MT3620 real-time core repeatedly transmits a simple message over a UART. These messages can be read in terminal application on a PC using a USB-to-serial adapter. By default, it uses the real-time core's dedicated UART, but if your hardware doesn't expose this UART's TX pin, then the sample can be altered to use a different UART.

To use this sample, clone the repository locally if you haven't already done so:

  ```shell
  git clone https://github.com/Azure/azure-sphere-samples.git
  ```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A breakout board and USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time core UART to a USB port on your PC. 
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) to display the output.

## Prep your device

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More > Run as administrator**. 
1. At the command prompt, issue the following command:

   ```shell
   azsphere device enable-development --EnableRTCoreDebugging
   ```

   This command must be run as administrator when you enable real-time core debugging because it installs USB drivers for the debugger.
1. Close the window after the command completes because administrator privilege is no longer required.  
    **Note:** As a best practice, you should always use the lowest privilege that can accomplish a task.

## Set up hardware to display output

To set up the hardware to display output from the Hello World sample, follow these steps.

1. Connect GND on the breakout adapter to Header 3, pin 2 (GND) on the MT3620 RDB.
1. Connect RX on the breakout adapter to Header 3, pin 6 (real-time core TX) on the MT3620 RDB.
1. Attach the breakout adapter to a USB port on your PC.
1. Start Device Manager.
1. Select **View > Devices by container**.
1. Look for your adapter and note the number of the assigned COM port.
1. On the PC, start the terminal emulator and open a serial terminal with the following settings: 115200-8-N-1 and the COM port assigned to your adapter.

## Build and run the sample

### Building and running the sample with Visual Studio
  
1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This step automatically performs the manual packaging steps. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (RTCore)**.
1. Press F5 to start the application with debugging.

### Building and running the sample from the command line

See [Build and debug an RTApp from the command line](https://docs.microsoft.com/azure-sphere/app-development/rtapp-manual-build).

## Observe the output

1. The connected terminal emulator should display output from the HelloWorld_RTApp_MT3620_Baremetal program. The program sends the following words at one-second intervals:

   `Tick`  
`Tock`
