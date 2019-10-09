# Sample: MT3620 high level application - PWM

The PWM_HighLevelApp sample  demonstrates how to use PWM in a simple digital-to-analog conversion application on an MT3620 device.

It varies the brightness of an LED by incrementally varying the duty cycle of the output pulses from the PWM.

[!NOTE]
Minimum and maximum period and duty cycle will vary depending on the hardware you use. For example, The MT3620 reference board’s PWM modulators run at 2 MHz with 16 bit on/off compare registers. This imposes a minimum duty cycle of 500 ns, and an effective maximum period of approximately 32.77 ms. Consult the data sheet for your specific device for details.

The sample uses the following Azure Sphere libraries.

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

## Setup

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt window, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so, by entering the following line in the Azure Sphere Developer Command Prompt window:

   `azsphere device prep-debug`

## Running the sample

1. Find the PWM_HighLevelApp sample in the PWM folder.
1. In Visual Studio, open PWM_HighLevelApp.sln.
1. Press F5 to compile and build the solution and load it onto the device for debugging.

LED1 (green on the RDB) will gradually increase in brightness until it reaches maximum, after which it will turn off and the cycle will repeat.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
