# Sample: System Time

This sample C application demonstrates how to manage the system time and to use the hardware [RTC (real time clock)](https://docs.microsoft.com/azure-sphere/app-development/rtc).

The system time is changed whenever button A is pressed, and it is synchronized to the hardware RTC whenever button B is pressed.

The sample uses the following Azure Sphere libraries and requires [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

|Library   |Purpose  |
|----------|---------|
|log       |  Displays messages in the Visual Studio Device Output window during debugging  |
|gpio      |  Digital input for buttons  |
|rtc       |  Synchronizes the hardware RTC with the current system time  |
|networking | Gets and sets network interface configuration |


## Prerequisites

 This sample requires the following hardware:

- Azure Sphere MT3620 board
- CR2032 coin cell battery

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../Hardware/README.md). Battery support may differ for your hardware; check with the manufacturer for details. 

You must perform these steps before you continue:

- Connect your Azure Sphere device to your computer.
- Ensure that the coin cell battery is not installed and the J3 jumper is set to the 3v3 position. For more information, see the [MT3620 development board user guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-supply).
- Complete the steps to [install Azure Sphere](https://docs.microsoft.com/azure-sphere/install/install).
- Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device enable-development`

## To prepare the sample

1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the SystemTime sample.

## To build and run the sample

### Building and running the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

### Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Change the system time without updating the hardware RTC

**Note:** If the device is connected to the internet, the system time may be overwritten by NTP (Network Time Protocol) service. To prevent the time from being overwritten by the NTP service, ensure that the device is not connected to the internet.

1. Disable the internet connection on the MT3620 device.
1. Note the current system time in the debug output window, displayed in both UTC and the local time zone. The sample uses Pacific Standard Time (PST) as the local time zone.
1. Press button A to advance the time by three hours.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time returned to the original time that you first observed.

## Change the system time and store it in the hardware RTC

1. Press button A to advance the time by three hours.
1. Press button B to store the new time in the hardware RTC.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time was maintained.

## Remove power from the hardware RTC

1. Stop the application.
1. Unplug the device.
1. Wait at least ten seconds and then plug the cable back into the device.
1. Wait at least five more seconds and then restart the application.
1. Verify that the system time is set to January 2000. This is the default time when the hardware RTC is first powered on.

## Power the hardware RTC from a battery

1. Stop the application.
1. Unplug the device. 
1. Insert the battery into the holder underneath the development board and set the J3 jumper into the BAT position so that it covers pins 1 and 2. For more information, see the [MT3620 development board user guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-supply). **Important:** The MT3620 will not start if you set the J3 jumper to the BAT position and the battery is dead or uninstalled.
1. Plug the cable back into the device.
1. Wait five seconds and then restart the application.
1. Press button A to advance the time by three hours.
1. Press button B to store the new time in the hardware RTC.
1. Stop the application.
1. Unplug the device.
1. Wait at least ten seconds and then plug the cable back into the device.
1. Wait at least five more seconds and then restart the application.
1. Verify that the system time was maintained after power loss.
