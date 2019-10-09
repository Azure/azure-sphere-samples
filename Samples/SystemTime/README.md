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

   `azsphere device prep-debug`

## To build and run the sample

1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the SystemTime sample.
1. In Visual Studio, open SystemTime.sln and press F5 to compile, build, and load the solution onto the device for debugging.

### Troubleshooting the Azure Sphere app

- Visual Studio returns the following error if the application fails to compile:

   `1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

   This error may occur for many reasons. Most often, the reason is that you did not clone the entire Azure Sphere Samples repository from GitHub. The samples depend on the hardware definition files that are supplied in the Hardware folder of the repository.

### To get detailed error information

By default, Visual Studio may only open the Error List panel, so that you see error messages like this:

`1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

To get more information, open the Build Output window. To open the window, select **View->Output**, then choose **Build** on the drop-down menu. The Build menu shows additional detail, for example:

```
1>------ Rebuild All started: Project: AzureIoT, Configuration: Debug ARM ------
1>main.c:36:10: fatal error: hw/sample_hardware.h: No such file or directory
1> #include <hw/sample_hardware.h>
1>          ^~~~~~~~~~~~~~~~~~~~~~
1>compilation terminated.
1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.
1>Done building project "AzureIoT.vcxproj" -- FAILED.
========== Rebuild All: 0 succeeded, 1 failed, 0 skipped ==========
```

In this case, the error is that hardware definition files aren't available.

The **Tools -> Options -> Projects and Solutions -> Build and Run** panel provides further controls for build verbosity.

### Change the system time without updating the hardware RTC

**Note:** If the device is connected to the internet, the system time may be overwritten by NTP (Network Time Protocol) service. To prevent the time from being overwritten by the NTP service, ensure that the device is not connected to the internet.

1. Disable the internet connection on the MT3620 device.
1. Note the current system time in the debug output window, displayed in both UTC and the local time zone. The sample uses Pacific Standard Time (PST) as the local time zone.
1. Press button A to advance the time by three hours.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time returned to the original time that you first observed.

### Change the system time and store it in the hardware RTC

1. Press button A to advance the time by three hours.
1. Press button B to store the new time in the hardware RTC.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time was maintained.

### Remove power from the hardware RTC

1. Stop the application.
1. Unplug the device.
1. Wait at least ten seconds and then plug the cable back into the device.
1. Wait at least five more seconds and then restart the application.
1. Verify that the system time is set to January 2000. This is the default time when the hardware RTC is first powered on.

### Power the hardware RTC from a battery

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

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).