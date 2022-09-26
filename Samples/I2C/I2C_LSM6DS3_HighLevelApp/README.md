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

This sample demonstrates how to use [I2C with Azure Sphere](https://learn.microsoft.com/azure-sphere/app-development/i2c) in a high-level application. The sample displays data from an accelerometer connected to an MT3620 development board through I2C (Inter-Integrated Circuit). Once per second the application calls the [Applibs I2C APIs](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-i2c/i2c-overview) to retrieve the accelerometer data. It then calls [Log_Debug](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/function-log-debug) to display the data.

By default, this sample is configured to use an external accelerometer—the [ST LSM6DS3](https://www.mouser.co.uk/datasheet/2/389/dm00133076-1798402.pdf). It is not configured to use the on-board sensors found on some development boards, such as the [ST LSM6DS0](https://www.st.com/resource/en/datasheet/LSM6DSO.pdf) on the Avnet Starter Kit. To run the sample using the Avnet MT3620 Starter Kit and the on-board ST LSM6DSO accelerometer, see [Use the Avnet MT3620 Starter Kit and its on-board accelerometer](#use-the-avnet-mt3620-starter-kit-and-its-on-board-accelerometer).

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [i2c](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-i2c/i2c-overview) | Manages Inter-Integrated Circuit (I2C) interfaces. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging. |

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

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- ST LSM6DS3 accelerometer
- 2 x 10K ohm resistors
- A breadboard (recommended because this sample requires wiring from multiple sources to the same pin and the use of pull-up resistors)
- Jumper wires to connect the boards

## Setup

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://learn.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.09 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *I2C_LSM6DS3_HighLevelApp* sample in the *I2C* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/i2c/).
1. Set up your hardware to display output from the sample. For instructions, see [Set up hardware to display output](https://learn.microsoft.com/azure-sphere/install/qs-multicore-dev#set-up-hardware-to-display-output).
1. Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

    **Note:** This sample uses ISU2 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

    ![Connection diagram for ST LSM6DS3 and MT3620](./media/i2cwiring.png)

### Use the Avnet MT3620 Starter Kit and its on-board accelerometer

If you're using the Avnet MT3620 Starter Kit and its on-board ST LSM6DSO accelerometer instead of the ST LSM6DS3 external accelerometer, make the following changes in the sample:

1. Open the `I2C\main.c` source code file:
    1. Replace `0x69` with `0x6C` in the following code:

        ```c
        static const uint8_t expectedWhoAmI = 0x69;
        ```

    1. Replace `SAMPLE_LSM6DS3_I2C` with `SAMPLE_LSM6DSO_I2C` in the following code:

        ```c
        i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);
        ```

1. Open the `app_manifest.json` file and replace `$SAMPLE_LSM6DS3_I2C` with `$SAMPLE_LSM6DSO_I2C` in the following code:

    ```json
    "I2cMaster": [ "$SAMPLE_LSM6DS3_I2C" ]
    ```

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

When you run the application, it reads the accelerometer WHO_AM_I register. The returned value (0x69 for the LSM6DS3) is compared with the application's **expectedWhoAmI** constant to verify that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct I2C interface. For details on the LSM6DS3 registers, see the [ST LSM6DS3 data sheet](https://www.mouser.co.uk/datasheet/2/389/dm00133076-1798402.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still and observe the accelerometer output in the **Output Window**. It should indicate a vertical acceleration of approximately +1g. Once the data from the accelerometer CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Output Window**. The vertical acceleration should change from approximately +1g to approximately -1g.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
