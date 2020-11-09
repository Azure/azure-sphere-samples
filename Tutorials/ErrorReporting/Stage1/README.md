# Stage 1: Application crashes

The goal of this stage is to provide insights on how to analyze the error reported when an application crashes. The application in this stage intentionally crashes when you press button A. You can download the error report to analyze the error messages and identify the cause of the crash.

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A, button B, and LED 2 on the device |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents

| File/folder | Description                               |
|-------------|-------------------------------------------|
| Stage1      | Tutorial source code and VS project files |
| README.md   | This readme file                          |

## Prerequisites

The tutorial requires the following:

* Hardware:
[Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

   **Note:** By default, this tutorial targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the tutorial for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

* Network connectivity: This tutorial requires internet connectivity on your device and your computer. The tutorial runs over a [Wi-Fi connection to the internet](https://docs.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device). To use Ethernet instead, make the following changes:

   1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
   1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
   1. Add the following line to the Capabilities section of the app_manifest.json file:

      `"NetworkConfig" : true`
   1. In main.c, ensure that the global constant `networkInterface` is set to "eth0". In source file main.c, search for the following line:

      `static const char networkInterface[] = "wlan0";`

      Change this line to:

      `static const char networkInterface[] = "eth0";`
   1. In main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

      ```c
      int err = Networking_SetInterfaceState(networkInterface, true);
      if (err == -1) {
         Log_Debug("Error setting interface state %d\n",errno);
         return -1;
      }
      ```

## Prepare the tutorial

1. Ensure that your Azure Sphere device is connected to your computer, and your computer and Azure Sphere device are connected to the internet using Wi-Fi or Ethernet for communicating with the Azure Sphere Security Service.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the Stage 1 tutorial in the Tutorials/ErrorReporting folder.

## Build a sample application

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Observe the output

1. LED2 on the MT3620 starts to blink blue.

1. Press button A to change the LED color to green. The application crashes.
The call to DeferenceNull in the code causes the application to crash before it can change the LED color.

   If the app was deployed from the command line without a debugger, the OS restarts it each time it fails. If the app was deployed from Visual Studio or Visual Studio Code, the OS stops when it fails and a `Segmentation fault` dialog is displayed.
1. Press the Reset button to restart the application.
1. Repeat steps 1 - 3 as desired.
1. Error reports are automatically uploaded from the device to the Azure Sphere Security Service. This occurs approximately once every 24 hours, or on device reboot. Wait for a few minutes for the error report to become available for download.
1. [Generate and download the error report](https://docs.microsoft.com/azure-sphere/deployment/interpret-error-data#generate-and-download-error-report).

## Interpret the error report

Open the downloaded CSV file and look for descriptions that contain the following:

* AppCrash
* [Application component ID](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#app-show-status)
* `signo=11` or `signal_code=3`

The descriptions look similar to the following:

`AppCrash (pc=213ECC62; lr=2136E6F7; sp=BE96DC28; signo=11; errno=0; code=0; component_id=a60a27ff-8936-4a49-b73f-70861db03d43)`

`AppCrash (exit_status=11; signal_status=11; signal_code=3; component_id=a60a27ff-8936-4a49-b73f-70861db03d43; image_id=db81a79a-7ec4-435d-9cac-b2bfe2f2e38b)`

If multiple crashes occur within a window of time during which event data are aggregated, the count is displayed in the Event Count column.
For more information about the errors and other events, see [Collect and interpret error data](https://docs.microsoft.com/azure-sphere/deployment/interpret-error-data).

## Fix the application

We recommend that you fix the code in Stage 1 and note the behavior of the fixed application.

To fix the application:

1. Comment out the DeferenceNull() call on line 154 in the main.c file.

1. Re-run the application and verify that when you press button A the color of the LED alternates between blue and green.

Proceed to Stage 2 of the tutorial for the correct implementation of the application. Observe that the behavior of the fixed application in Stage 1 and application in Stage 2 is the same.