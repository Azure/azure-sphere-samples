# Stage 2: Application fixed

The goal of this stage is to provide insights on how to analyze the logs when the application runs successfully.
In this stage, the application runs as intended and the crash introduced in Stage 1 is removed.
When you press button A the LED color alternates between blue and green, and exits successfully when you press button B.

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|--|--|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A, button B, and LED 2 on the device |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents

| File/folder | Description                             |
|-------------|-----------------------------------------|
| Stage2      | Tutorial source code and VS project files |
| README.md   | This readme file                        |

## Prerequisites

The tutorial requires the following:

* Hardware: [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

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
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.01 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

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