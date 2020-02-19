# Sample: Power Down (High-level app)

This sample application demonstrates how to force an Azure Sphere device into the [Power Down](https://docs.microsoft.com/azure-sphere/app-development/power-down) state and then periodically wakes up the device to check for [OS and app updates](https://docs.microsoft.com/azure-sphere/app-development/power-down#force-power-down-and-updates).

This application performs the following cycle of operations:

1. Blinks an LED red, indicating that the device is awake.
1. After a specified amount of time (60 seconds), puts the device into the Power Down state.
1. After a specified number of repetitions of this cycle, the device stays awake for the OS to check for updates. The repetition count is specified by `MaxCyclesUntilAllowUpdateCheckComplete`.
1. After any updates are processed, the cycle repeats from the beginning.

**Note:** To ensure updates have completed before the Power Down state is requested, the sample app makes use of three [system event notifications](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/enum-sysevent-events): **SysEvent_Events_NoUpdateAvailable,** **SysEvent_Events_UpdateStarted,** and **SysEvent_Events_UpdateReadyForInstall.** These notifications can be used to inform the app about the status of downloads and updates.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|---------|---------|
|[log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages in the Visual Studio Device Output window during debugging  |
|[GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview)    | Used for GPIO (general-purpose input/output) pin communication      |
|[eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Used for LED blink, Power Down, and other timers |
|[sysevent](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Used to register for system event notifications about updates so that the app can make sure update checks have completed before the Power Down state is requested |
|[powermanagement](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-power/power-overview) | Used to manage the power state of the device |
|[storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview) | Used to keep track of how many Power Down/boot cycles have occurred since an update check |

## Contents

| File/folder | Description |
|-------------|-------------|
| Powerdown_HighLevelApp       | Sample source code and project files |
| README.md     | This readme file |

## Prerequisites

This sample requires the following hardware:

- Azure Sphere device

    **NOTE:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md). You might also need to wire the boards differently; check with your hardware manufacturer for details.

- CR2032 battery to power the real-time clock (RTC): Insert the battery into the battery holder on the bottom of the RDB as shown:
![Connection diagram for pulling PMU_EN low on RDB v1.0](./media/RTC_battery_RDB.png)

- Jumper connecting the first and second pins, **not** the second and third pins (i.e. left and middle, **not** middle and right). This will allow the RTC to use the battery as its power source instead of the system 3.3V supply. See the bottom-left corner of the image below.

- Female-female header (if using RDB v1.0): [MT3620 User Guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide) to determine whether you have RDB v1.0. If you have RDB v1.0, you will need to tie the PMU_EN pin low to enable the MT3620 to Power Down. Connect the female header from pin 10 on H3 (PMU_EN) to pin 2 on H4 (GND), as shown on the right side of this image:

  ![Image showing where to insert the RTC battery on the RDB](./media/PMU_EN_low_RDB.png)

- In-line USB current meter (optional): You can connect an in-line USB current meter between your RDB and your PC to measure the current consumption of the RDB. This is helpful for validating that the RDB has entered Power Down by observing the decrease in current consumption displayed on the meter.

    **Note:** Most USB current meters will work, although be sure to get one with a data passthrough feature so you can deploy apps and debug while it's connected. One such USB current meter is the [StarTech USB Voltage and Current Tester Kit](https://www.startech.com/Cables/USB-2.0/USB-Adapters/usb-voltage-current-tester-kit~USBAUBSCHM) (pictured below).

  ![Image showing USB current meter setup](./media/Current_Meter_Setup.jpg)

    **Note:** Additional on-board circuitry (the FTDI interface and so forth) is also powered from the main power supply. When the chip is placed in Power Down mode, the overall current consumption of the board will not drop to the expected MT3620 Power Down levels because the FTDI consumes between 10-80mA, depending on its connection activity with the USB Host device. As such, the RDB is helpful for validating that software is correctly placing the chip in Power Down mode, but is less useful for validating overall power consumption of the hardware design.

## Prepare the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 20.01 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK for Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [the Azure Sphere SDK for Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux).
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/install/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so, using this command:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the PowerDown_HighLevelApp sample in the Powerdown folder.

## Build and run the sample

You can build and run the sample in these environments:

- [Microsoft Windows using Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
- [Microsoft Windows using the CLI](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
- [Linux from the CLI](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

### Build and run the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.

1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.

1. Press F5 to start the application with debugging. See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.

    **Note:** After a Power Down/wake cycle, the VS debugger will no longer be attached to the Azure Sphere device. This means you will also no longer see debug console output after the device wakes up. As a workaround, redeploy the sample app after the device wakes up to restore debugging functionality and debug console output.

### Build and run the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

### Build and run the sample from the Linux CLI

You can also build Azure Sphere applications from the Linux command line. To learn how, see [Quickstart: Build the Hello World sample application on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application in Linux.

## Test Power Down

- When you run the application, LED #2 will begin blinking red for 60 seconds. If you are using a USB current meter, it should show approximately 0.15A during this time.

- After 60 seconds of blinking, the device will enter Power Down, at which time you should see the LED stop blinking. If using the USB current meter, you should see the current reading drop to approximately 0.05A while the device is in the Power Down state.
<br><br>**Note:** The RDB has additional components that will cause it to consume much more current than a production device would when in the Power Down state. This test is only intended for observing the relative current drop to validate that your application is entering Power Down.

- The device will stay in Power Down for `powerdownResidencyTime` seconds (the default is 10 seconds). Change this constant value in the app to configure how long the device stays in Power Down.

- After `powerdownResidencyTime` seconds, the device will wake up. You should see the reading on the current meter rise back up to approximately 0.15A and LED #2 begin to blink red again as the app starts.

## Test Update Checks

- Every `MaxCyclesUntilAllowUpdateCheckComplete` number of Power Down/boot cycles, the device will stay awake longer to check for updates. You will see LED #2 start to blink green while the device begins the update process.

- When checking for updates, the app will wait until one of three events occurs:

  1. The app receives a `SysEvent_Events_NoUpdateAvailable` event notification, indicating the update check has completed and no updates are available. In this case, the device will Power Down, assuming that the red LED blinked at least 60 seconds. If the blink cycle hadn't completed its 60 second time span, the device will wait for the cycle to end before entering Power Down.

  1. It receives a `SysEvent_Events_UpdateStarted` event. This means an update is available and has begun downloading. The green LED will begin to blink more slowly to indicate an update is downloading. The device will stay awake while the update downloads until one of two things occurs:

      1. The update finishes downloading, indicated by the reception of a `SysEvent_Events_UpdateReadyForInstall` event notification. The LED stops blinking and remains switched on to indicate the download has completed. After approximately 2 seconds, the device will reboot to apply the update.

      1. The update has not finished downloading after `waitForUpdateDownloadTimeout` seconds have elapsed. In this case, the app will Power Down to conserve energy. For example, this might happen if an unreliable network connection is causing the download to take an excessive amount of time. The app will try to resume the download again the next time it wakes up.

  1. An update check has not completed. This means that neither of the update event notifications (`SysEvent_Events_NoUpdateAvailable` nor `SysEvent_Events_UpdateStarted`) were received after `waitForUpdateCheckTimeout` seconds. In this case, the app will Power Down to conserve energy and try to check for updates again the next time it wakes up.

## Next steps

- To learn more about power management and the Power Down state on Azure Sphere, see [Manage Power Down state for Azure Sphere devices](https://docs.microsoft.com/azure-sphere/app-development/power-down).
- For more details about Power Down that are specific to the RDB, see the [Power Down section of the MT3620 RDB User Guide](https://docs.microsoft.com/azure-sphere/hardware/mt3620-user-guide#power-down-mode).
- To learn about hardware considerations to ensure your design can take advantage of Power Down, see [the Power Down considerations section of the MT3620 hardware notes documentation](https://docs.microsoft.com/azure-sphere/hardware/mt3620-hardware-notes#power-down-considerations).

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).