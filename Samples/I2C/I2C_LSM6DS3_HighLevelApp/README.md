---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – I2C
urlFragment: I2C
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
description: "Demonstrates how to use the I2C (Inter-Integrated Circuit) interface with Azure Sphere in a high-level application."
---

# Sample: I2C high-level app

This sample demonstrates how to use [I2C with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/i2c) in a high-level application. The sample displays data from an accelerometer connected to an MT3620 development board through I2C (Inter-Integrated Circuit). Once per second the application calls the [Applibs I2C APIs](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-i2c/i2c-overview) to retrieve the accelerometer data. It then calls [Log_Debug](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/function-log-debug) to display the data.

By default, this sample is configured to use an external accelerometer—the [ST LSM6DS3](https://www.st.com/en/mems-and-sensors/lsm6ds3.html). It is not configured to use the on-board sensors found on some development boards, such as the [ST LSM6DS0](https://www.st.com/resource/en/datasheet/LSM6DSO.pdf) on the Avnet Starter Kit. To run the sample using the Avnet MT3620 Starter Kit and the on-board LSM6DSO accelerometer, see [Changes required to use the Avnet MT3620 Starter Kit and its on-board LSM6SDO accelerometer](#changes-required-to-use-the-avnet-mt3620-starter-kit-and-its-on-board-lsm6sdo-accelerometer).

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Device Output window during debugging  |
|i2c    | Manages I2C interfaces |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere device

   **NOTE:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md). You might also need to wire the boards differently; check with your hardware manufacturer for details.

- ST LSM6DS3 accelerometer
- 2 x 10K ohm resistors
- We recommend a breadboard because this sample requires wiring from multiple sources to the same pin and the use of pull-up resistors.
- Jumper wires to connect the boards.

## Prepare the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.01 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *I2C_LSM6DS3_HighLevelApp* sample in the *I2C* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/i2c/).

## Set up hardware to display output

To prepare your hardware to display output from the sample, see "Set up hardware to display output" for [Windows](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) or [Linux](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output).

## Set up the ST LSM6DS3 connections

Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

**Note:** this sample uses ISU2 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

![Connection diagram for ST LSM6DS3 and MT3620](./media/i2cwiring.png)

## Changes required to use the Avnet MT3620 Starter Kit and its on-board LSM6SDO accelerometer

1. Open main.c
    1. Search for `static const uint8_t expectedWhoAmI = 0x69;` and replace `0x69` with `0x6C`
    1. Search for `i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);` and replace `SAMPLE_LSM6DS3_I2C` with `SAMPLE_LSM6DSO_I2C`

1. Open app_manifest.json
    1. Search for `"I2cMaster": [ "$SAMPLE_LSM6DS3_I2C" ]` and replace `$SAMPLE_LSM6DS3_I2C` with `$SAMPLE_LSM6DSO_I2C`

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Test the sample

When you run the application, it reads the accelerometer WHO_AM_I register. The returned value (0x69 for the LSM6DS3) is compared with the application's `expectedWhoAmI` constant to verify that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct I2C interface. For details on the LSM6DS3 registers, see the [ST LSM6DS3 data sheet](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still and observe the accelerometer output in the **Output Window**. It should indicate a vertical acceleration of approximately +1g. Once the data from the accelerometer CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Output Window**. The vertical acceleration should change from approximately +1g to approximately -1g.
