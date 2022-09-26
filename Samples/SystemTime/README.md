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

This sample application demonstrates how to manage the system time and the hardware real-time clock (RTC). The system time is changed whenever button A is pressed, and it is synchronized to the hardware RTC whenever button B is pressed.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventLoop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages digital input for buttons. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging. |
| [networking](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets and sets the network interface configuration. |
| [rtc](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-rtc/rtc-overview) | Synchronizes the hardware real-time clock (RTC) with the current system time. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakePresets.json`   | CMake presets file, which contains the information to configure the CMake project. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for deploying and debugging the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../HardwareDefinitions/README.md) file.

- CR2032 coin cell battery (not installed)
- J3 jumper is set to the 3v3 position (pins 2 and 3 of J3 are connected)

## Setup

1. Ensure that the coin cell battery is not installed and the J3 jumper is set to the 3v3 position (pins 2 and 3 of J3 are connected). For more information, see the [MT3620 development board user guide](https://learn.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-supply).
1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://learn.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 22.09 or above. To verify the SDK version, open a command-line interface using PowerShell, Windows command prompt, or Linux command shell, and run the **azsphere show-version** command. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *SystemTime* sample in the *SystemTime* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/systemtime/).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

To test the sample, perform the following operations, which are described in the sections below:

- [Change the system time without updating the hardware RTC](#change-the-system-time-without-updating-the-hardware-rtc).
- [Change the system time and store it in the hardware RTC](#change-the-system-time-and-store-it-in-the-hardware-rtc).
- [Remove power from the hardware RTC](#remove-power-from-the-hardware-rtc).
- [Power the hardware RTC from a battery](#power-the-hardware-rtc-from-a-battery).

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
1. Insert the battery into the holder underneath the development board and set the J3 jumper into the BAT position so that it covers pins 1 and 2. For more information, see the [MT3620 development board user guide](https://learn.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-supply). **Important:** The MT3620 will not start if you set the J3 jumper to the BAT position and the battery is dead or uninstalled.
1. Plug the cable back into the device.
1. Wait five seconds and then restart the application.
1. Press button A to advance the time by three hours.
1. Press button B to store the new time in the hardware RTC.
1. Stop the application.
1. Unplug the device.
1. Wait at least ten seconds and then plug the cable back into the device.
1. Wait at least five more seconds and then restart the application.
1. Verify that the system time was maintained after power loss.

## Further reference
You may also be interested in the following related projects on the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery):

- [SetTimeFromLocation](https://github.com/Azure/azure-sphere-gallery/blob/main/SetTimeFromLocation) | Project that shows how to use Reverse IP lookup to get location information, then obtain time for location, and set device time.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
- To learn more about the real-time clock (RTC), see [Manage time and use the RTC](https://learn.microsoft.com/azure-sphere/app-development/rtc).
