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
description: "Demonstrates how to use the Azure IoT APIs to communicate with Azure IoT Central or Azure IoT Hub."
---

# Sample: Azure IoT

This sample demonstrates how to use the Azure IoT SDK C APIs in an Azure Sphere application to communicate with [Azure IoT Hub](https://docs.microsoft.com/azure/iot-fundamentals/iot-introduction) or [Azure IoT Central](https://docs.microsoft.com/azure/iot-central/core/overview-iot-central).

**IMPORTANT**: Before you can compile and run the sample, you must configure an Azure IoT Hub application, and customize the sample to connect to it. Carefully follow the instructions in this README and in its linked topics.

This sample application implements an example *thermometer* that works with Azure IoT Hub as follows:

1. Sends simulated *temperature* telemetry data at regular intervals, with a timestamp.
1. Sends a simulated *thermometer moved* telemetry event (with timestamp) when button B is pressed.
1. Reports a read-only string *serial number* device twin.
1. Synchronizes a read/write boolean *Thermometer Telemetry Upload Enabled* device twin.
   - This status is reflected by one of the LEDs on the MT3620 development board.
   - This status can be turned on/off via the cloud or on the device itself by pressing button A.
1. Implements a *display alert* direct method. For example, a cloud solution could call this method when it receives a temperature reading that is higher than a given threshold.
1. Declares that it implements the *Azure Sphere Example Thermometer* model, consisting of this telemetry, device twin, and direct method by sending its [Azure IoT Plug and Play (PnP)](https://docs.microsoft.com/azure/iot-pnp/overview-iot-plug-and-play) model ID upon connection.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons A and B and LED 4 on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Determines whether the device is connected to the internet. |
| [storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview) | Opens the certificate file that is used to authenticate to the IoT Edge device. |

## Contents

| File/folder                    | Description |
|--------------------------------|-------------|
| `app_manifest.json`            | Application manifest file, which describes the resources. |
| `CMakeLists.txt`               | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`           | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`               | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`                  | The license for this sample application. |
| `main.c`                       | Main C source code file. |
| `README.md`                    | This README file. |
| `READMEAddDPS.md`              | Instructions for changing the device authentication method to use Azure IoT Hub device provisioning service instead of direct authentication. |
| `READMEAddIoTEdge.md`          | Instructions for adding an IoT Edge device that provides a filtering and data processing layer between your Azure Sphere device and IoT Hub. |
| `READMEAddWebProxy.md`          | Instructions for adding web proxy support to the sample |
| `READMEStartwithIoTCentral.md` | Instructions for customizing the sample to run with Azure IoT Central. |
| `READMEStartwithIoTHub.md`     | Instructions for customizing the sample to run with Azure IoT Hub. |
| `.vscode`                      | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions`          | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

This sample requires the following software and hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../HardwareDefinitions/README.md) file.

- Azure Sphere SDK version 22.02 or higher. At the command prompt, run [**azsphere show-version**](https://docs.microsoft.com/azure-sphere/reference/azsphere-show-version) to check your SDK version. Install the latest [Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.

- An Azure subscription. If your organization does not already have a subscription, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

## Setup

1. Set up your Azure Sphere device and development environment as described in [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *AzureIoT* sample in the *AzureIoT* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/browse/?expanded=azure&products=azure-sphere).
1. Connect your Azure Sphere device to your computer by USB.
1. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet.
1. Open a command-line interface and [enable application development](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) on your device if you have not already done so by entering the **azsphere device enable-development** command.
1. Configure networking on your device. You must either [set up WiFi](https://docs.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

## Build and run the sample

You can build this sample to run with either either Azure IoT Hub or Azure IoT Central:

- Follow the [Azure IoT Hub instructions](./READMEStartWithIoTHub.md) to build the sample and run it with Azure IoT Hub.

   - To change your device authentication method to use the device provisioning service instead of direct authentication, see [Connect via Azure IoT Hub device provisioning service](./READMEAddDPS.md).

   - To add an IoT Edge device that provides a filtering and data processing layer between your Azure Sphere device and IoT Hub, see [Connect via Azure IoT Edge](./READMEAddIoTEdge.md).

- Follow the [Azure IoT Central instructions](./READMEStartWithIoTCentral.md) to build the sample and run it with Azure IoT Central.

## Further reference
You may also be interested in the following related project(s) on the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery):

- [AzureIoT_StoreAndForward](https://github.com/Azure/azure-sphere-gallery/blob/main/AzureIoT_StoreAndForward) | This project shows how to add telemetry store and forward capability to the AzureIoT sample.
- [AzureIoTMessageWithProperties](https://github.com/Azure/azure-sphere-gallery/blob/main/AzureIoTMessageWithProperties) | A project that shows how to add custom properties to an Azure IoT Hub telemetry message, which can then be used for message routing.

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- To access a collection of unmaintained scripts, utilities, and functions, see the [Azure Sphere Gallery](https://github.com/Azure/azure-sphere-gallery#azure-sphere-gallery).
