---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Power Down
urlFragment: PowerDown
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
description: "Demonstrates how to force an Azure Sphere device into the Power Down state and wake up the device to check for updates."
---

# Sample: Power Down high-level app

This sample application demonstrates how to force an Azure Sphere device into the [Power Down](https://docs.microsoft.com/azure-sphere/app-development/power-down) state and wake up the device to check for [OS and app updates](https://docs.microsoft.com/azure-sphere/app-development/power-down#force-power-down-and-updates).

To ensure that updates have completed before the Power Down state is requested, the sample application makes use of three [system event notifications](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/enum-sysevent-events): **SysEvent_Events_NoUpdateAvailable**, **SysEvent_Events_UpdateStarted**, and **SysEvent_Events_UpdateReadyForInstall**. It uses these notifications to find out about the status of downloads and updates.

The sample performs the following cycle of operations:

![Powerdown sample flowchart](./media/Power-Down-Sample-Flow-Chart-15.png)

**Note:** This sample writes data to and erases data from the flash. Be aware that repeatedly updating the flash eventually wears it out and makes it invalid.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for LED blink, Power Down, and other timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview)    | Communicates with general-purpose input/outputs (GPIOs). |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     | Displays messages in the Device Output window during debugging. |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Checks if the device is connected to a network and if the time is synchronized. |
| [powermanagement](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-power/power-overview) | Manages the power state of the device. |
| [storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview) | Keeps track of when an update check happened. |
| [sysevent](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Registers for system event notifications about updates. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- CR2032 coin cell battery to power the real-time clock (RTC) (Optional): Insert the battery into the battery holder on the bottom of the RDB as shown in the following image:

   ![Connection diagram for pulling PMU_EN low on RDB v1.0](./media/RTC_battery_RDB.png)

- Jumper block to set the power source for the RTC:

  - To power the RTC from the main 3.3V power supply, use the jumper block to connect pins 2 and 3 of jumper J3 (J3.2, J3.3).
  - To power the RTC from a coin cell battery, use the jumper block to connect pins 1 and 2 of Jumper J3 (J3.1, J3.2) as shown in the bottom left corner of the RDB in the following image.

     **Note:** For versions v1.6 and later of the RDB, when pins J3.1 and J3.2 are connected, the RTC will be powered from the main 3.3V power supply when present or from the coin cell battery when the main supply is not present.

- Female-female header (if using RDB v1.0): To determine which version of the RDB you have, see the [MT3620 User Guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-down-mode). If you have RDB v1.0, you will need to tie the PMU_EN pin low to enable the MT3620 to Power Down. Connect the female header from pin 10 on H3 (PMU_EN) to pin 2 on H4 (GND), as shown on the right side of the following  image:

   ![Image showing where to insert the RTC battery on the RDB](./media/PMU_EN_low_RDB.png)

- In-line USB current meter (optional): You can connect an in-line USB current meter between your RDB and your PC to measure the current consumption of the RDB. This is helpful for validating that the RDB has entered Power Down by observing the decrease in current consumption displayed on the meter.

    **Note:** Although most USB current meters will work, get one with a data passthrough feature so that you can deploy apps and debug while it's connected. One such USB current meter is the [StarTech USB Voltage and Current Tester Kit](https://www.startech.com/Cables/USB-2.0/USB-Adapters/usb-voltage-current-tester-kit~USBAUBSCHM) (pictured below).

   ![Image showing USB current meter setup](./media/Current_Meter_Setup.jpg)

    **Note:** Additional on-board circuitry (the FTDI interface and so forth) is also powered from the main power supply. When the chip is placed in Power Down state, the overall current consumption of the board will not drop to the expected MT3620 Power Down levels because the FTDI consumes between 10-80mA, depending on its connection activity with the USB Host device. As such, the RDB is helpful for validating that software is correctly placing the chip in Power Down state, but is less useful for validating overall power consumption of the hardware design.

## Setup

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this setup previously, ensure you have Azure Sphere SDK version 21.10 or above. Open a [Azure Sphere command-line tool](https://docs.microsoft.com/azure-sphere/reference/overview), and run [**azsphere show-version**](https://docs.microsoft.com/azure-sphere/reference/azsphere-show-version) to check. Install [the Azure Sphere SDK for Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [the Azure Sphere SDK for Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux).
1. Connect your Azure Sphere device to your PC by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *Powerdown_HighLevelApp* sample in the *Powerdown* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/powerdown/).

1. Configure networking on your device. You must either [set up WiFi](https://docs.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

To test the sample, follow the instructions in the [Test power down](#test-power-down) and [Test update checks](#test-update-checks) sections below.

Note the following:

- When the MT3620 is in Power Down state, it will be unresponsive to CLI commands or attempts to deploy a new or updated image from Visual Studio and Visual Studio Code. The reset button on the development board will not work, and recovery will also not work while the board is in this state. See the [MT3620 Hardware Notes](https://docs.microsoft.com/azure-sphere/hardware/mt3620-hardware-notes#power-down-considerations) for more information.
- After a Power Down/wake cycle, the debugger will no longer be attached to the Azure Sphere device. This means you will also no longer see debug console output after the device wakes up. As a workaround, redeploy the sample app after the device wakes up to restore debugging functionality and debug console output.

### Test power down

The device repeatedly cycles through three states: Start-up, Update, and Power Down.

1. Start-up

    When you start the application or the device wakes up from the Power Down state LED #2 will begin to blink red and continue to blink for 60 seconds. If you are using a USB current meter, it should show approximately 0.15A during this time.

    When you first run the application, the last update time is set to 1970. If the device is not connected to a network, it might check for updates at every power-down cycle, depending on the local time of your device.

1. Update

   When the device enters Update LED #2 continues to blink but changes color from red to green. In this state the blink rate of LED #2 indicates the following conditions:

   - Rapid - The app is waiting for notification of an update. If a notification is not received within 60 seconds the device goes into Power Down state.
   - Slow - An update is available and has begun downloading
   - Steady (on but not blinking) - The download has completed and the device will reboot to apply the download.

   See the [Test update checks](#test-update-checks) section below for details.

1. Power Down

   When the device enters Power Down, LED #2 will stop blinking and turn off.

   If you are using the USB current meter, you should see the current reading drop to approximately 0.05A.

   **Note:** The RDB has additional components that will cause it to consume much more current than a production device would when in the Power Down state. This test is only intended for observing the relative current drop to validate that your application is entering Power Down.

   The device will stay in Power Down for **powerdownResidencyTime** seconds (the default is 10 seconds). Change this constant value in the app to configure how long the device stays in Power Down.

   After **powerdownResidencyTime** seconds, the device will wake up. You should see the reading on the current meter rise back up to approximately 0.15A.

### Test update checks

 When checking for updates, the app will wait until one of three events occurs:

  1. The app receives a **SysEvent_Events_NoUpdateAvailable** event notification, indicating the update check has completed and no updates are available. In this case, the device will wait until the end of the 60 second interval and enter Power Down.

  1. It receives a **SysEvent_Events_UpdateStarted** event. This means an update is available and has begun downloading. The green LED will begin to blink more slowly to indicate an update is downloading. The device will stay awake while the update downloads until one of two things occurs:

      - The update finishes downloading, indicated by the reception of a **SysEvent_Events_UpdateReadyForInstall** event notification. The LED stops blinking but remains on to indicate the download has completed. After approximately 2 seconds, the device will reboot to apply the update.

      - The update has not finished downloading after **waitForUpdatesToDownloadTimerInterval** seconds have elapsed. In this case, the app will Power Down to conserve energy. For example, this might happen if an unreliable network connection is causing the download to take an excessive amount of time. The app will try to resume the download again the next time it wakes up.

  1. An update check has not completed. This means that neither of the update event notifications (**SysEvent_Events_NoUpdateAvailable** nor **SysEvent_Events_UpdateStarted**) were received after **businessLogicCompleteTimerInterval** seconds. In this case, the app will Power Down to conserve energy and try to check for updates again the next time it wakes up.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- To learn about how to use power profiles to adjust the balance between performance and energy savings, see [Set power profiles](https://docs.microsoft.com/azure-sphere/app-development/set-power-profiles).
