# Stage 2: Application fixed

The goal of this stage is to provide insights on how to analyze the logs when the application runs successfully.
In this stage, the application runs as intended and the crash introduced in Stage 1 is removed.
When you press button A the LED color alternates between blue and green, and exits successfully when you press button B.

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A, button B, and LED 2 on the device. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages during debugging. |

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

## Prerequisites

The tutorial requires the following:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- Internet connectivity on your device and your computer.

## Prepare the tutorial

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Ensure that you have Azure Sphere SDK version 24.03 or above. At the command prompt, run `az sphere show-sdk-version` to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Ensure that the [Azure CLI](https://learn.microsoft.com/cli/azure/install-azure-cli) is installed. At a minimum, the Azure CLI version must be 2.45.0 or later.
1. Install the [Azure Sphere extension](https://learn.microsoft.com/azure-sphere/reference/cli/overview?view=azure-sphere-integrated).
1. Enable application development, if you have not already done so, by entering the `az sphere device enable-development` command in the [command prompt](https://learn.microsoft.com/azure-sphere/reference/cli/overview?view=azure-sphere-integrated).

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the Stage 2 tutorial in the Tutorials/ErrorReporting folder.

1. Configure networking on your device. You must either [set up WiFi](https://learn.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://learn.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

## Build a sample application

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Observe the output

1. LED2 on the MT3620 starts to blink blue.

1. Press button A to change the LED color to green. When you press button A, the color alternates between blue and green.

   The DeferenceNull() function introduced in Stage 1 has been removed from main.c and the application no longer crashes.
1. Press button B to exit the application successfully.
1. Repeat steps 1 - 3 as desired.
1. Error reports are automatically uploaded from the device to the Azure Sphere Security Service. This occurs approximately once every 24 hours, or on device reboot. Wait for a few minutes for the error report to become available for download.
1. [Generate and download the error report](https://learn.microsoft.com/azure-sphere/deployment/interpret-error-data#generate-and-download-error-report).

## Interpret the error report

Open the downloaded CSV file and look for descriptions that contain the following:

* AppExit
* [Application component ID](https://learn.microsoft.com/azure-sphere/app-development/component-id)

The descriptions look similar to the following:

`AppExit (exit_code=1; component_id=a7d43534-9f43-4e89-8e5f-44c985abe034; image_id=db81a79a-7ec4-435d-9cac-b2bfe2f2e38b)`

AppCrash in Stage 1 and AppExit in Stage 2 differ in these ways:

* When an application crashes, the description contains `AppCrash`, `signal_status`, `signal_code` and `exit_status`
* When an application exits successfully, the description contains `AppExit` and `exit_code`.


If you pressed B multiple times within a window of time during which event data are aggregated, the count is displayed in the Event Count column. For more information about the errors and other events, see [Collect and interpret error data](https://learn.microsoft.com/azure-sphere/deployment/interpret-error-data).
