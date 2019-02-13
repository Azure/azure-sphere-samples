# Sample: LSM6DS3 I2C

This sample C application demonstrates how to use [I2C with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/i2c). The sample displays data from an ST LSM6DS3 accelerometer connected to an MT3620 development board through I2C (Inter-Integrated Circuit). The accelerometer data is retrieved every second and is displayed by calling the [Applibs I2C APIs](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/i2c/i2c-overview).

The sample uses the following Azure Sphere libraries and includes [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|i2c    | Manages I2C interfaces |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere MT3620 board
- [ST LSM6DS3](https://www.st.com/en/mems-and-sensors/lsm6ds3.html)
- 2 x 10K ohm resistors
- We recommend a breadboard because this sample requires wiring from multiple sources to the same pin and the use of pull-up resistors.
- Jumper wires to connect the boards.

Make the following connections between the ST LSM6DS3 and MT3620 dev boards. Make sure that power is disconnected while you wire the boards.

**Note:** this sample uses ISU2 on the MT3620 board; however, you can use another ISU by adjusting the wiring, the code, and the application manifest.

![Connection diagram for ST LSM6DS3 and MT3620](./media/i2cwiring.png)

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.02 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device prep-debug`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the LSM6DS3_I2C sample.
1. In Visual Studio, open LSM6DS3_I2C.sln and press F5 to compile and build the solution and load it onto the device for debugging.

## Test the accelerometer data

When you run the application, it reads the WHO_AM_I register from the accelerometer. This should return the known value 0x69, which confirms that the MT3620 can successfully communicate with the accelerometer. If this fails, verify that the devices are wired correctly, and that the application opened the correct I2C interface. For details on the registers, see the [ST LSM6DS3 data sheet](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf).

After displaying the initial values, the application configures the accelerometer and then displays the vertical acceleration every second.

To test the accelerometer data:

1. Keep the device still, and observe the accelerometer output in the **Output Window**. Once the data from the CTRL3_C register is displayed, the output should repeat every second.

1. Turn the accelerometer upside down and observe the updated data in the **Output Window**. The vertical acceleration should change from approximately +1g to approximately -1g.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).