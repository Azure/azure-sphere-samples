---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – SPI
urlFragment: SPI
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
description: "Demonstrates how to use the SPI (Serial Peripheral Interface) with Azure Sphere in a high-level application."
---

# Sample: SPI high-level app

This sample demonstrates how to use [SPI with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/spi) in a high-level application.

The sample displays data from an ST LSM6DS3 accelerometer connected to an MT3620 development board through the Serial Peripheral Interface (SPI). The accelerometer data is retrieved every second and is displayed by calling the [Applibs SPI APIs](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-spi/spi-overview).

The sample uses the following Azure Sphere libraries.

| Library   | Purpose |
|-----------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) |  Displays messages in the Device Output window during debugging. |
| [spi](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-spi/spi-overview) | Manages the Serial Peripheral Interfaces (SPIs). |

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

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- [ST LSM6DS3 inertial measurement unit (IMU)](https://www.st.com/en/mems-and-sensors/lsm6ds3.html)
- A breadboard (recommended because this sample requires wiring from multiple sources to the same pin)
- Jumper wires to connect the boards

## Setup

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 21.10 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *SPI_LSM6DS3_HighLevelApp* sample in the *SPI* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/spi/).

### Set up the ST LSM6DS3 connections

Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

**Note:** By default, this sample is configured to use an external ST LSM6DS3 and isn't configured to use the onboard sensors found on some development boards, such as the ST LSM6DS0 on the Avnet Starter Kit. This sample uses ISU1 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

![Connection diagram for ST LSM6DS3 and MT3620](./media/spiwiring.png)

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

When you run the application, it reads the WHO_AM_I register from the accelerometer. This should return the known value 0x69, which confirms that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct SPI interface. For details on the registers, see the [ST LSM6DS3 data sheet](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still, and observe the accelerometer output in the **Device Output** window. Once the data from the CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Device Output** window. The vertical acceleration should change from approximately +1g to approximately -1g.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
