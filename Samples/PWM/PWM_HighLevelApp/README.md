---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – PWM
urlFragment: PWM
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
description: "Demonstrates how to use the PWM (pulse-width modulator) interface in a simple digital-to-analog conversion application."
---

# Sample: PWM high-level app

This sample demonstrates how to use the pulse-width modulator (PWM) interface in a simple digital-to-analog conversion application on an MT3620 device.

The sample varies the brightness of an LED by incrementally varying the duty cycle of the output pulses from the PWM.

**Note:** Minimum and maximum period and duty cycle will vary depending on the hardware you use. For example, The MT3620 reference board's PWM modulators run at 2 MHz with 16 bit on/off compare registers. This imposes a minimum duty cycle of 500 ns, and an effective maximum period of approximately 32.77 ms. Consult the data sheet for your specific device for details.

 The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging. |
| [pwm](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-pwm/pwm-overview) | Manages the pulse-width modulators (PWMs). |

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

The sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

## Setup

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 23.05 or above. To verify SDK version, open a command-line interface using PowerShell, Windows command prompt, or Linux command shell, and run the `azsphere show-version` command. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *PWM_HighLevelApp* sample in the *PWM* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/pwm/).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

The output messages are displayed in the **Device Output** window during debugging.

LED1 (green on the MT3620 RDB) gradually increases in brightness until it reaches maximum, after which it turns off and the cycle repeats.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
