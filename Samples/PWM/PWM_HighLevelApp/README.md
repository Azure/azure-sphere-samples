# Sample: MT3620 high level application - PWM

The PWM_HighLevelApp sample  demonstrates how to use PWM in a simple digital-to-analog conversion application on an MT3620 device.

It varies the brightness of an LED by incrementally varying the duty cycle of the output pulses from the PWM.

[!NOTE]
Minimum and maximum period and duty cycle will vary depending on the hardware you use. For example, The MT3620 reference board’s PWM modulators run at 2 MHz with 16 bit on/off compare registers. This imposes a minimum duty cycle of 500 ns, and an effective maximum period of approximately 32.77 ms. Consult the data sheet for your specific device for details.

The sample uses the following Azure Sphere libraries and requires [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

| Library | Purpose |
|---------|---------|
| [pwm](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-pwm/pwm-overview) | Manages PWMs |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |

## Contents

| File/folder | Description |
|-------------|-------------|
| `PWM_HighLevelApp`       |Sample source code and VS project files|
| `PWM_HighLevelApp.sln` |VS solution file|
| `README` | This README file. |
| `LICENSE`   | The license for the sample. |

## Prerequisites

1. Clone the entire Azure Sphere samples repository locally:

     ```sh
         git clone https://github.com/Azure/azure-sphere-samples.git
     ```

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

## To prepare the sample

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt window, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows
 as needed.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so, by entering the following line in the Azure Sphere Developer Command Prompt window:

   `azsphere device enable-development`

## To build and run the sample

### Building and running the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.

### Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Observe the sample

LED1 (green on the RDB) will gradually increase in brightness until it reaches maximum, after which it will turn off and the cycle will repeat.
