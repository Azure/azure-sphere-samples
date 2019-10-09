
# Sample: MT3620 real-time capable application - UART

This sample demonstrates how to communicate over UART in a real-time capable application (RTApp) for the MT3620. For more information about RTApps, see the [applications overview](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).

It provides the same user experience as the high level application [UART sample](../UART_HighLevelApp/README.md). However, it uses two UARTS. It uses the ISU0 UART, which is also available to high-level applications, and the debug UART, which is specific to the real-time core. It also uses a general-purpose timer (GPT) on the real-time core. For more information about timers, see [General purpose timers](https://docs.microsoft.com/azure-sphere/app-development/use-peripherals-rt.md##general-purpose-timers). [Use peripherals in a real-time capable application](https://docs.microsoft.com/azure-sphere/app-development/use-peripherals-rt) provides more detail about the use of UARTs in an RTApp.

A loopback connector joins the TX and RX pins on the ISU0 UART. The debug UART is connected to a terminal program on the PC via a USB-to-serial adapter.

When you press button A, the application transmits a message on ISU0 UART. The application then receives the message on ISU0 UART, via the loopback connector. The application then prints the received message to the debug UART. You can view that message on the terminal program that is running on the PC.

The debug UART is used in other real-time samples. In those samples, the application polls when it sends data via the UART. This sample demonstrates a more sophisticated use of the UARTs. It uses in-memory buffers to both send and receive data, and responds to interrupts rather than polling. This makes the sample a more useful basis for applications that do not want to block when they send or receive data.

To use this sample, clone the repository locally if you haven't already done so:

```
     git clone https://github.com/Azure/azure-sphere-samples.git
```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A breakout board and USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time core UART to a USB port on your PC. 
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/)) to display the output.


## To build and run the sample

**Prep your device**

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More > Run as administrator**.
1. At the command prompt, issue the following command:

   ```sh
   azsphere dev prep-debug --EnableRTCoreDebugging
   ```

   This command must be run as administrator when you enable real-time core debugging because it installs USB drivers for the debugger.
1. Close the window after the command completes because administrator privilege is no longer required.  
    **Note:** As a best practice, you should always use the lowest privilege that can accomplish a task.

**Set up hardware to display output**

To set up the hardware to display output from the Hello World sample, follow these steps.

1. Connect GND on the breakout adapter to Header 3, pin 2 (GND) on the MT3620 RDB.
1. Connect RX on the breakout adapter to Header 3, pin 6 (real-time core TX) on the MT3620 RDB.
1. Attach the breakout adapter to a USB port on your PC.
1. Determine which COM port the adapter uses on the PC. 
    1. Start Device Manager. 
    1. Select **View > Devices by container**, 
    1. Look for your adapter and note the number of the assigned COM port.
1. On the PC, start the terminal emulator and open a serial terminal with the following settings: 115200-8-N-1 and the COM port assigned to your adapter.

**Set up the loopback connection**

1. Use a jumper header to connect Header 2 pin 1 (RXD0) to Header 2 pin 3 (TXD0).

**Build and deploy the application**
  
1. Start Visual Studio. From the **File** menu, select **Open > Make...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This step automatically performs the manual packaging steps. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (RTCore)**.
1. Press F5 to start the application with debugging. The following message is sent to the serial terminal:

   ```UART_RTApp_MT3620_BareMetal
   App built on: May 24 2019 00:29:45
   Install a loopback header on ISU0, and press button A to send a message.
   ```  

   After you press button A the following message will be sent:  

   ```
   UART received 12 bytes: 'RTCore: Hell'.
   UART received 10 bytes: 'o world!
   ```

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).