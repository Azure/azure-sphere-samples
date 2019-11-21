# Sample: LSM6DS3 SPI

This sample C application demonstrates how to use [SPI with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/spi). The sample displays data from an ST LSM6DS3 accelerometer connected to an MT3620 development board through SPI (Serial Peripheral Interface). The accelerometer data is retrieved every second and is displayed by calling the [Applibs SPI APIs](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-spi/spi-overview).

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|spi    | Manages SPI interfaces |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere MT3620 board
- [ST LSM6DS3](https://www.st.com/en/mems-and-sensors/lsm6ds3.html)
- We recommend a breadboard because this sample requires wiring from multiple sources to the same pin.
- Jumper wires to connect the boards.

Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

   **Note:** By default, this sample is configured to use an external LSM6DS3, and isn't configured to use the onboard sensors found on some development boards, such as the LSM6DS0 on the Avnet Starter Kit. This sample uses ISU1 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

![Connection diagram for ST LSM6DS3 and MT3620](./media/spiwiring.png)

## To prepare the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows
 as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the SPI_LSM6DS3_HighLevelApp sample in the SPI folder.

## To build and run the sample

### Building and running the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.

### Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Test the accelerometer data

When you run the application, it reads the WHO_AM_I register from the accelerometer. This should return the known value 0x69, which confirms that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct SPI interface. For details on the registers, see the [ST LSM6DS3 data sheet](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still, and observe the accelerometer output in the **Output Window**. Once the data from the CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Output Window**. The vertical acceleration should change from approximately +1g to approximately -1g.
