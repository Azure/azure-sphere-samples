# Sample: HelloWorld_HighLevelApp

This sample shows how to use CMake to build an Azure Sphere high-level application. It simply blinks an LED, so that you can verify that the Azure Sphere device and tools are installed and set up correctly.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [gpio.h](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) |Contains functions and types that interact with GPIOs.  |
| [log.h](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |

## Contents

| File/folder         | Description |
|---------------------|-------------|
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

## Setup

1. Ensure that your Azure Sphere device is connected to your computer and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.09 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *HelloWorld_HighLevelApp* sample in the *HelloWorld* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/helloworld/).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Observe the output

LED1 on the MT3620 will begin to blink red.

You will need the component ID to stop or start the application. To get the component ID, enter the **azsphere device app show-status** command. Azure Sphere will return the component ID (a GUID) and the current state (running, stopped, or debugging) of the application:

```
azsphere device app show-status
12345678-9abc-def0-1234-a76c9a9e98f7: App state: running
```

To stop the application, enter the following command:

```
azsphere device app stop --component-id <component ID>
```

To restart the application, enter the following command:

```
azsphere device app start --component-id <component ID>
```
