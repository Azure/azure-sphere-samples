
# Sample: Inter-core communication

This sample demonstrates how to exchange messages between applications running on the high-level and real-time capable cores.

Once per second the high-level application sends a message "Hello-World-%d", where %d is an incrementing counter to the real-time capable application. The real-time capable application prints the received data, converts any upper-case characters to lower-case and vice versa, and echoes the message back to the high-level application.

The high-level application uses the following Azure Sphere libraries and includes [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta):

|Library   |Purpose  |
|---------|---------|
|application.h |Communicates with and controls real-time capable applications |
|log.h |Displays messages in the Visual Studio Device Output window during debugging |

The real-time capable features used in the sample are in Beta.

To use this sample, clone the repository locally if you haven't already done so:

```
git clone https://github.com/Azure/azure-sphere-samples.git
```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time capable core UART to a USB port on your PC. 
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/.)) to display the output.

## Prep your device

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.
1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More&gt;Run as administrator**.
1. At the command prompt, issue the following command:

   ```shell
   azsphere device enable-development --EnableRTCoreDebugging
   ```

   This command must be run as administrator when you enable real-time capable core debugging because it installs USB drivers for the debugger.
1. Close the window after the command completes because administrator privilege is no longer required.  
    **Note:** As a best practice, you should always use the lowest privilege that can accomplish a task.

## Set up hardware to display output

To set up the hardware to display output from the Hello World sample, follow these steps.

1. Connect GND on the USB-to-serial adapter to Header 3, pin 2 (GND) on the MT3620 RDB.
1. Connect RX on the USB-to-serial adapter to Header 3, pin 6 (real-time core TX) on the MT3620 RDB.
1. Attach the USB-to-serial adapter to a USB port on your PC.
1. Start Device Manager.
1. Select **View&gt;Devices by container**.
1. Look for your adapter and note the number of the assigned COM port.
1. On the PC, start the terminal emulator and open a serial terminal with the following settings: 115200-8-N-1 and the COM port assigned to your adapter.

## Build and run the apps

The applications in this sample run as partners. Make sure that they're designated as partners as described in [sideload more than one application](https://docs.microsoft.com/azure-sphere/app-development/sideload-app#sideload-more-than-one-application) so that sideloading one doesn't delete the other.

### Build and run the RTApp with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open&gt;CMake...** and navigate to the folder that contains the sample (../IntercoreComms_RTApp_MT3620_BareMetal).
1. Select the file CMakeLists.txt and then click **Open**.
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This step automatically performs the manual packaging steps. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (RTCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

### Build and run the RTApp from the command line

See [Build and debug an RTApp from the command line](https://docs.microsoft.com/azure-sphere/app-development/rtapp-manual-build).

### Build and run the high level application with Visual Studio

1. Start another instance of Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample (../IntercoreComms_HighLevelApp).
1. Select the file CMakeLists.txt and then click **Open**.
1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/troubleshooting.md) if you encounter errors.

### Build and run the high level application from the Windows CLI

To learn how to build an Azure Sphere application from the command line, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Observe the output

The high-level application output will be displayed in the Output window in Visual Studio.

```sh
Remote debugging from host 192.168.35.1
High-level intercore application.
Sends data to, and receives data from the real-time core.
Sending: Hello-World-0
Received 13 bytes: hELLO-wORLD-0
Sending: Hello-World-1
Received 13 bytes: hELLO-wORLD-1
Sending: Hello-World-2
```

The real-time core application output will be sent to the serial terminal for display.

```sh
IntercoreComms_RTApp_MT3620_BareMetal
App built on: Nov 12 2019, 09:31:30
Received message of 33 bytes:
  Component Id (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:30
  Payload (13 bytes as text): Hello-World-0
Received message of 33 bytes:
  Component Id (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:31
  Payload (13 bytes as text): Hello-World-1
Received message of 33 bytes:
  Component Id (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:32
  Payload (13 bytes as text): Hello-World-2
```
