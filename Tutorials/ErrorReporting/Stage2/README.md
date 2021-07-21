# Stage 2: Application fixed

The goal of this stage is to provide insights on how to analyze the logs when the application runs successfully.
In this stage, the application runs as intended and the crash introduced in Stage 1 is removed.
When you press button A the LED color alternates between blue and green, and exits successfully when you press button B.

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A, button B, and LED 2 on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages during debugging. |

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

## Prerequisites

The tutorial requires the following:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- Internet connectivity on your device and your computer. The tutorial runs over a [Wi-Fi connection to the internet](https://docs.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device). To use Ethernet instead, follow the [Ethernet setup instructions](../../../ethernet-setup-instructions.md).

## Prepare the tutorial

1. Ensure that your Azure Sphere device is connected to your computer, and your computer and Azure Sphere device are connected to the internet using Wi-Fi or Ethernet for communicating with the Azure Sphere Security Service.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.07 or above. At the command prompt, run [**azsphere show-version**](https://docs.microsoft.com/azure-sphere/reference/azsphere-show-version?tabs=cliv1) to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   ```
   azsphere device enable-development
   ```

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the Stage 2 tutorial in the Tutorials/ErrorReporting folder.

## Build a sample application

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Observe the output

1. LED2 on the MT3620 starts to blink blue.

1. Press button A to change the LED color to green. When you press button A, the color alternates between blue and green.

   The DeferenceNull() function introduced in Stage 1 has been removed from main.c and the application no longer crashes.
1. Press button B to exit the application successfully.
1. Repeat steps 1 - 3 as desired.
1. Error reports are automatically uploaded from the device to the Azure Sphere Security Service. This occurs approximately once every 24 hours, or on device reboot. Wait for a few minutes for the error report to become available for download.
1. [Generate and download the error report](https://docs.microsoft.com/azure-sphere/deployment/interpret-error-data#generate-and-download-error-report).

## Interpret the error report

Open the downloaded CSV file and look for descriptions that contain the following:

* AppExit
* [Application component ID](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#app-show-status)

The descriptions look similar to the following:

`AppExit (exit_code=1; component_id=a7d43534-9f43-4e89-8e5f-44c985abe034; image_id=db81a79a-7ec4-435d-9cac-b2bfe2f2e38b)`

AppCrash in Stage 1 and AppExit in Stage 2 differ in these ways:

* When an application crashes, the description contains `AppCrash`, `signal_status`, `signal_code` and `exit_status`
* When an application exits successfully, the description contains `AppExit` and `exit_code`.


If you pressed B multiple times within a window of time during which event data are aggregated, the count is displayed in the Event Count column. For more information about the errors and other events, see [Collect and interpret error data](https://docs.microsoft.com/azure-sphere/deployment/interpret-error-data).
