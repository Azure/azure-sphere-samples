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
| `PWM_HighLevelApp`       |Sample source code and project files|
| `README` | This README file. |
| `LICENSE`   | The license for the sample. |

## Prerequisites

1. Clone the entire Azure Sphere samples repository locally:

     ```sh
         git clone https://github.com/Azure/azure-sphere-samples.git
     ```

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

## Prepare the sample

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

1. Ensure that your Azure Sphere device is connected to your computer,  andyour computer is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.01 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows
 as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build, deploy this sample:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

### Observe the output

 LED1 (green on the RDB) will gradually increase in brightness until it reaches maximum, after which it will turn off and the cycle will repeat.

 You will need the component ID to stop or start the application. To get the component ID, enter the command `azsphere device app show-status`. Azure Sphere will return the component ID (a GUID) and the current state (running, stopped, or debugging) of the application.

```sh
C:\Build>azsphere device app show-status
12345678-9abc-def0-1234-a76c9a9e98f7: App state: running
```

To stop the application enter the command `azsphere device app stop -i <component ID>`.

To restart the application enter the command `azsphere device app start -i <component ID>`.
