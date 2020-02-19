# Sample: GPIO (High-level app)

This application does the following:

- Provides access to one of the LEDs on the MT3620 development board using GPIO
- Uses a button to change the blink rate of the LED

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/en-gb/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
| GPIO_HighLevelApp       |Sample source code and VS project files |
|  ||
| README.md | This readme file |

## Prerequisites

The sample requires the following hardware:

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.01 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the GPIO_HighLevelApp sample in the GPIO folder.

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build and deploy this sample:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

## Observe the output

 LED1 on the MT3620 begins blinking red.

 Press button A repeatedly to cycle through the 3 possible blink rates.

You will need the component ID to stop or start the application. To get the component ID, enter the command `azsphere device app show-status`. Azure Sphere will return the component ID (a GUID) and the current state (running, stopped, or debugging) of the application.

```sh
C:\Build>azsphere device app show-status
12345678-9abc-def0-1234-a76c9a9e98f7: App state: running
```

To stop the application enter the command `azsphere device app stop -i <component ID>`.

To restart the application enter the command `azsphere device app start -i <component ID>`.
