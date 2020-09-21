## Sample: I2C_LSM6DS3_HighLevelApp

This sample C application demonstrates how to use [I2C with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/i2c) in a high-level application. The sample displays data from an ST LSM6DS3 accelerometer connected to an MT3620 development board through I2C (Inter-Integrated Circuit). The accelerometer data is retrieved every second, by calling the [Applibs I2C APIs](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-i2c/i2c-overview), and then displayed by calling [Log_Debug](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/function-log-debug).

To run the sample using the Avnet MT3620 Starter Kit and the on-device LSM6DSO accelerometer, see [Changes required to use the Avnet MT3620 Starter Kit and its built-in LSM6SDO accelerometer](#changes-required-to-use-the-avnet-mt3620-starter-kit-and-its-built-in-lsm6sdo-accelerometer).

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|i2c    | Manages I2C interfaces |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere device

   **NOTE:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md). You might also need to wire the boards differently; check with your hardware manufacturer for details.

- [ST LSM6DS3](https://www.st.com/en/mems-and-sensors/lsm6ds3.html)

   **Note:** By default, this sample is configured to use an external LSM6DS3 device.  The Avnet Starter Kits include an on-board [LSM6DS0](https://www.st.com/en/mems-and-sensors/lsm6dso.html) sensor.  This example will work on the Avnet Starter Kit (to read the on-board LSMDS0 device) with some minor modifications, see the detailed instructions in the section titled "Changes required to use the Avnet MT3620 Starter Kit and its built-in LSM6SDO accelerometer."  Since the LSM6DS3 and LSD6DS0 devices would be on the same I2C interface (ISU2), and the two devices contain overlapping register address', the Avnet Starter Kit is not able to use the external LSM6DS3 device.

- 2 x 10K ohm resistors
- We recommend a breadboard because this sample requires wiring from multiple sources to the same pin and the use of pull-up resistors.
- Jumper wires to connect the boards.

## Prepare the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.07 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the I2C_LSM6DS3_HighLevelApp sample in the I2C folder.

## Set up hardware to display output

To prepare your hardware to display output from the sample, see "Set up hardware to display output" for [Windows](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) or [Linux](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output).

## Set up the ST LSM6DS3 connections

Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

**Note:** this sample uses ISU2 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

![Connection diagram for ST LSM6DS3 and MT3620](./media/i2cwiring.png)

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Test the sample

When you run the application, it reads the WHO_AM_I register from the accelerometer. This should return the known value 0x69, which confirms that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct I2C interface. For details on the registers, see the [ST LSM6DS3 data sheet](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still and observe the accelerometer output in the **Output Window**. Once the data from the CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Output Window**. The vertical acceleration should change from approximately +1g to approximately -1g.

## Changes required to use the Avnet MT3620 Starter Kit and its built-in LSM6SDO accelerometer

1. Open main.c
    1. Search for `static const uint8_t expectedWhoAmI = 0x69;` and replace `0x69` with `0x6C`
    1. Search for `i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);` and replace `SAMPLE_LSM6DS3_I2C` with `SAMPLE_LSM6DSO_I2C`

1. Open app_manifest.json
    1. Search for `"I2cMaster": [ "$SAMPLE_LSM6DS3_I2C" ]` and replace `$SAMPLE_LSM6DS3_I2C` with `$SAMPLE_LSM6DSO_I2C`
