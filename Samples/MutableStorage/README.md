---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Mutable storage
urlFragment: MutableStorage
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
description: "Demonstrates how to manage storage on an Azure Sphere device."
---

# Sample: Mutable storage

This sample application illustrates how to manage [storage](https://docs.microsoft.com/azure-sphere/app-development/storage) on an Azure Sphere device.

When you press button A, the sample opens a persistent data file on the device, increments the value in it, and closes the file. When you press button B, the sample deletes the file. The file persists if the application exits or is updated. However, if you delete the application by using the **azsphere device sideload delete** command, the file is deleted as well.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|gpio |  Enables use of buttons and LEDs |
|log     |  Displays messages in the Device Output window during debugging  |
|storage    | Manages persistent user data |

## Contents

| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
|launch.vs.json |Describes how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prepare the sample

**Note:**: By default, this sample targets MT3620 reference development board (RDB)  hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the README file in the HardwareDefinitions folder.

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MutableStorage sample.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

## Test the sample

When the application starts:
1. Press button A to open and write to a file.
1. Press the button repeatedly to increment the value in the file.
1. Press button B to delete the file.
