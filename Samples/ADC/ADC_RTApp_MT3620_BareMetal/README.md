# Sample: MT3620 real-time capable application - ADC

This sample application demonstrates how to do analog-to-digital conversion (ADC) on an MT3620 real-time core. It takes a single reading from ADC channel 0 every second, outputting the result over the real-time core's debug UART. These messages can be viewed in a terminal application on a PC using a USB-to-serial adapter.

A potentiometer voltage divider is used to provide a simple variable voltage source that ranges between 0V and 2.5V (the MT3620 reference voltage).

The ADC has 8 channels. The ADC channels and GPIO41 through GPIO48 map to the same pins on the MT3260. The ADC pins are allocated in a block. That is, if your application uses the ADC then all 8 pins are allocated for use as ADC inputs. None of them can be used for GPIO. For example, if the app_manifest.json specifies both “ADC0” (referring to the ADC controller not ADC channel 0) and GPIO42, then deployment fails with the following message:
```
error: Could not deploy application to device: Application manifest specifies peripherals that use the same underlying pin.
```
On the MT3620 RDB only 4 of the 8 ADC channels are available on header pins (other MT3620 hardware may expose different ADC pins). The remaining 4 are reserved for GPIO LEDs. However, deployment will still fail if your application uses both ADC and one or more of the status LEDs listed in the following table.

|MT3620 Physical Pin   |MT3620 GPIO  |MT3620 RDB Function  |
|---------|---------|---------|
| H2.11    |  GPIO41 | ADC0 |
| H2.13  |    GPIO42  | ADC1 |
|H2.12  | GPIO43 |ADC2 |
| H2.14 | GPIO44 |ADC3 |
|-  | GPIO45 |Red Status LED |
| - | GPIO46 | Green Status LED|
|-  | GPIO47 |Blue Status LED |
| - | GPIO48 |Networking LED |


The real-time capable features used in the sample are in Beta.

To use this sample, clone the repository locally if you haven't already done so:

```
     git clone https://github.com/Azure/azure-sphere-samples.git
```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. [10 Kohm Potentiometer](https://www.digikey.com/product-detail/en/bourns-inc/3386P-1-103TLF/3386P-103TLF-ND/1232547?_ga=2.193850989.1306863045.1559007598-536084904.1559007598).
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

**Set up the ADC connections**

1. Connect MT3620 dev board pin H2.2 (GND) to an outer terminal of the potentiometer.
1. Connect both pin 1 and pin 2 of jumper J1 to the other outer terminal of the potentiometer. This connects the MT3620 2.5 V output to the ADC VREF pin and to the potentiometer.  
1. Connect MT3620 dev board pin H2.11 (GPIO41 / ADC0) to the center terminal of the potentiometer.

**Build and deploy the application**
  
1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select CMakeLists.txt and then click **Open**
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This step automatically performs the manual packaging steps. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (RTCore)**.
1. Press F5 to start the application with debugging. The application will sample and display the voltage divider output once every second. Adjust the potentiometer and observe that the displayed value changes.

```
--------------------------------
ADC_RTApp_MT3620_BareMetal
App built on: May 27 2019, 16:00:58
2.478
2.473
2.459
2.487
2.490
2.319
0.380
0.000
0.001
0.007
2.079
2.279
2.283
2.212
2.040
2.034
2.036
2.034
2.035
```

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).