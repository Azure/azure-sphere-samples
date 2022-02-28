---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – ADC
urlFragment: ADC
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
description: "Demonstrates how to do analog-to-digital conversion on the MT3620 high-level core."
---

# Sample: ADC high-level app

This sample application demonstrates how to do analog-to-digital conversion in a high-level application.

The application samples and displays the output from a simple variable voltage source once per second. It uses the MT3620 analog-to-digital converter (ADC) to sample the voltage.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [adc](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-adc/adc-overview) | Manages the analog-to-digital converters (ADCs). |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

The sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- [10K ohm Potentiometer](https://www.digikey.com/product-detail/en/bourns-inc/3386P-1-103TLF/3386P-103TLF-ND/1232547?_ga=2.193850989.1306863045.1559007598-536084904.1559007598).

## Setup

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.02 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *ADC_HighLevelApp* sample in the *ADC* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/adc/).

### Set up the ADC connections

1. Connect MT3620 dev board pin H2.2 (GND) to an outer terminal of the potentiometer.
1. Connect both pin 1 and pin 2 of jumper J1 to the other outer terminal of the potentiometer. This connects the MT3620 2.5 V output to the ADC VREF pin and to the potentiometer.
1. Connect MT3620 dev board pin H2.11 (GPIO41 / ADC0) to the center terminal of the potentiometer.

![ADC connections](./media/ADC-WireUp.png)

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

The ADC output is displayed in the output terminal or **Device Output** window during debugging. Adjust the potentiometer and observe that the displayed value changes, as shown in the following example output.

```
Show output from: Device Output
The out sample value is 2.500 V
The out sample value is 2.483 V
The out sample value is 2.337 V
The out sample value is 2.055 V
```

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- To learn more about how to interact with an ADC peripheral by using the simplified Azure Sphere functions or the advanced Linux IOCTLs, see the [ADC code snippets](https://github.com/Azure/azure-sphere-samples/tree/main/CodeSnippets/Peripherals/ADC).
- For information about using ADCs, see [Use ADCs in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/adc).
