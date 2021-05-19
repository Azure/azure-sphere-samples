---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Azure IoT
urlFragment: AzureIoT
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
- path: ethernet-setup-instructions.md
  target: ethernet-setup-instructions.md
description: "Demonstrates how to use the Azure IoT APIs to communicate with Azure IoT Central or Azure IoT Hub."
---

# Sample: Azure IoT

This sample demonstrates how to use the Azure IoT SDK C APIs in an Azure Sphere application to communicate with [Azure IoT Hub](https://docs.microsoft.com/azure/iot-fundamentals/iot-introduction) or [Azure IoT Central](https://docs.microsoft.com/azure/iot-central/core/overview-iot-central).

**IMPORTANT**: Before you can compile and run the sample, you must configure an Azure IoT Hub application, and customize the sample to connect to it. Carefully follow the instructions in this README and in its linked topics.

## Overview

This sample application implements an example *thermometer* that works with Azure IoT Hub as follows:

1. Sends simulated *temperature* telemetry data at regular intervals.
1. Sends a simulated *thermometer moved* telemetry event when button B is pressed.
1. Reports a read-only string *serial number* device twin.
1. Synchronizes a read/write boolean *Thermometer Telemetry Upload Enabled* device twin.
   - This status is reflected by one of the LEDs on the MT3620 development board.
   - This status can be turned on/off via the cloud or on the device itself by pressing button A.
1. Implements a *display alert* direct method. For example, a cloud solution could call this method when it receives a temperature reading that is higher than a given threshold.
1. Declares that it implements the *Azure Sphere Example Thermometer* model, consisting of this telemetry, device twin, and direct method by sending its [Azure IoT Plug and Play (PnP)](https://docs.microsoft.com/azure/iot-pnp/overview-iot-plug-and-play) model ID upon connection.

## Application libraries

This sample uses the following Azure Sphere application libraries:

|Library   |Purpose  |
|---------|---------|
|[log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages in the Device Output window during debugging  |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Determines whether the device is connected to the internet |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons A and B and LED 4 on the device |
| [storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview) | Opens the certificate file that is used to authenticate to the IoT Edge device |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents

| File          | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. | `README.md`           | This README file. |
| `READMEStartwithIoTCentral.md`       | Instructions for customizing the sample to run with Azure IoT Central. |
| `READMEAddIoTEdge.md`          | Instructions for customizing the sample to run with an Azure IoT Edge device. |
| `READMEStartwithIoTHub.md`           | Instructions for customizing the sample to run with Azure IoT Hub. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

This sample requires the following software and hardware:

- **Azure Sphere SDK version 21.04 or higher**. At the command prompt, run [**azsphere show-version**](https://docs.microsoft.com/azure-sphere/reference/azsphere-show-version) to check your SDK version. Install the latest [Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- **An Azure subscription**. If your organization does not already have a subscription, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

   **Note**: By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, like the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../HardwareDefinitions/README.md).

## Preparation

Complete the following steps to prepare your environment:

1. Set up your Azure Sphere device and development environment as described in [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *AzureIoT* sample in the *AzureIoT* folder or download the zip file from the [Microsoft samples browser](/samples/azure/azure-sphere-samples/azureiot/).
1. Connect your Azure Sphere device to your computer by USB.
1. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet.
1. Open the [Azure Sphere command-line tool](https://docs.microsoft.com/azure-sphere/reference/overview) and [enable application development](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) on your device if you have not already done so by entering the following command:

   `azsphere device enable-development`

### Use Ethernet instead of Wi-Fi

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, follow the [Ethernet setup instructions](../../ethernet-setup-instructions.md).

## Run the sample

You can use either Azure IoT Hub or Azure IoT Central to configure the sample:

1. [Run the sample by connecting to an Azure IoT Hub](./READMEStartWithIoTHub.md).

   - To change your device authentication method to use device provisioning service instead of direct authentication, see [Connect via Azure IoT Hub device provisioning service](./READMEAddDPS.md).
   - To add an IoT Edge device that provides a filtering and data processing layer between your Azure Sphere device and IoT Hub, see [Connect via Azure IoT Edge](./READMEAddIoTEdge.md).

1. [Run the sample by connecting to an Azure IoT Central application](./READMEStartWithIoTCentral.md).

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- See the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery#azure-sphere-gallery), a collection of unmaintained scripts, utilities, and functions.
