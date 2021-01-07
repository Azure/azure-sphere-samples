---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – System time
urlFragment: SystemTime
extendedZipContent:
- path: HardwareDefinitions
  target: HardwareDefinitions
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to manage the system time and the hardware real-time clock."
---

# Sample: System time

This sample application demonstrates how to manage the system time and the hardware [RTC (real time clock)](https://docs.microsoft.com/azure-sphere/app-development/rtc). The system time is changed whenever button A is pressed, and it is synchronized to the hardware RTC whenever button B is pressed.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|----------|---------|
|log       |  Displays messages in the Device Output window during debugging  |
|gpio      |  Digital input for buttons  |
|rtc       |  Synchronizes the hardware RTC with the current system time  |
|networking | Gets and sets network interface configuration |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
|launch.vs.json |Describes how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere MT3620 board
- CR2032 coin cell battery

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../HardwareDefinitions/README.md). Battery support may differ for your hardware; check with the manufacturer for details. 

You must perform these steps before you continue:

- Connect your Azure Sphere device to your computer.
- Ensure that the coin cell battery is not installed and the J3 jumper is set to the 3v3 position. For more information, see the [MT3620 development board user guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-supply).
- Complete the steps to [install Azure Sphere](https://docs.microsoft.com/azure-sphere/install/overview).
- Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

## To prepare the sample

1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the SystemTime sample.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

### Change the system time without updating the hardware RTC

**Note:** If the device is connected to the internet, the system time may be overwritten by NTP (Network Time Protocol) service. To prevent the time from being overwritten by the NTP service, ensure that the device is not connected to the internet.

1. Ensure that the coin cell battery has been removed from its holder and the RTC is powered directly from the 3V3 on-board supply (by linking pins 2 and 3 of J3).
1. Disable the internet connection on the MT3620 device.
1. Note the current system time in the debug output window, displayed in both UTC and the local time zone. The sample uses Pacific Standard Time (PST) as the local time zone.
1. Press button A to advance the time by three hours.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time returned to the original time that you first observed.

### Change the system time and store it in the hardware RTC

1. Ensure that the coin cell battery has been removed from its holder and the RTC is powered directly from the 3V3 on-board supply (by linking pins 2 and 3 of J3).
1. Press button A to advance the time by three hours.
1. Press button B to store the new time in the hardware RTC.
1. Stop the application.
1. Press the reset button and wait at least five seconds.
1. Restart the application.
1. Verify that the system time was maintained.

### Remove power from the hardware RTC

1. Ensure that the coin cell battery has been removed from its holder and the RTC is powered directly from the 3V3 on-board supply (by linking pins 2 and 3 of J3).
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

