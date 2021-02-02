---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Deferred update
urlFragment: DeferredUpdate
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
description: "Demonstrates how a high-level application can defer application updates."
---

# Sample: Deferred update high-level-app

This sample demonstrates how to [defer updates](https://docs.microsoft.com/azure-sphere/app-development/device-update-deferral) to an Azure Sphere application. In this sample, you will do the following:

- Set up the Blink or Hello World application as an update for your device, so that it will eventually replace the Deferred Update application.
- Load the Deferred Update application onto the device.
- Use the device buttons to defer or accept the update.
- Observe that the Deferred Update application is replaced with the Blink/Hello World application when the update is accepted.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons and LEDs on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Monitors and dispatches events. |
| [sysevent](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Interacts with system event notifications. |

## Prerequisites

The sample requires the following hardware:

- Azure Sphere device

   **Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

- Azure Sphere SDK version 21.01 or above. To check, run **azsphere show-version** at the command prompt.

## Create the Blink application .imagepackage file

1. Connect the Azure Sphere device to your computer through USB and ensure that the device is connected to the internet.

1. Run `azsphere device enable-development`. This command sideloads the application debugging capabilities and GDB server, and moves the device to the Development device group. This command doesn't delete the application.

1. Follow the tutorial to [build a high-level application](https://docs.microsoft.com/azure-sphere/install/qs-blink-application).

1. Confirm that the application is deployed and running (LED1 blinks red).

1. Note the location of the Blink.imagepackage file. For example: `<path to your Blink folder>\Blink\out\ARM-Debug\Blink.imagepackage`.

1. Delete the application from the device.

    1. Run `azsphere device image list-installed` to display the component ID of the application.

    1. Run `azsphere device sideload delete --componentid <blink-component-id>` to delete the application.

## Prepare the device to receive updates

1. If you haven't already, run `azsphere product create --name MyProduct` at the command prompt. This command creates a product and the [standard device groups](https://docs.microsoft.com/azure-sphere/deployment/deployment-concepts#device-groups).

1. At the Azure Sphere command prompt, run `azsphere device update --productname MyProduct --devicegroupname "Field Test"` to move the device to the Field Test group.

1. Run `azsphere device wifi show-status` to verify that the Azure Sphere device is connected to your wifi network.

## Build and run the Deferred Update application

To build and run the Deferred Update sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

After the application starts up, LED2 will light up green to indicate that the Deferred Update application is running and updates will be deferred.

## Deploy the Blink.imagepackage file

Run `azsphere device-group deployment create --productname MyProduct --devicegroupname "Field Test" --filepath <path to your Blink folder>\Blink\out\ARM-Debug\Blink.imagepackage` to create a deployment for the Field Test device group.

## Update the device

An Azure Sphere device is not automatically notified when updates are available. It checks for updates when it first connects to the internet and at regular intervals (currently 24 hours) thereafter. 
The device will also check for updates when it is restarted.

You can update the device with the Deferred Update application running inside or outside the Visual Studio or Visual Studio Code integrated development environment (IDE), continuing with one of the options below. The sample uses the following LEDs.

| LED | Description |
|---------|---------|
| LED 3 | Blue if an update was received. |
| LED 2 | Green if updates are being deferred, yellow if updates are being accepted. |

### Outside an IDE

1. After you deploy the Blink.imagepackage file, restart the device.

   On restart the device will check for updates.

   After restart, LED 2 will initially light up green.

   After a short period, LED 3 will light up blue to indicate that an update is available.

1. Press button A to start the update.

   LED 2 will turn yellow to indicate that the update is being accepted and installed.

   After a few seconds, LED 2 and LED 3 will turn off as the Deferred Update application exits.

   LED 1 will blink red to indicate that the Blink/Hello World application was deployed, and the Deferred Update application and GDB server were removed.

1. Run `azsphere device image list-installed` to verify that the Blink/Hello World application is installed, and the Deferred Update application and GDB server are no longer installed.

### In an IDE

1. After you deploy the Blink.imagepackage, restart the device.

   On restart the device will check for updates.

   After restart, LED 2 will initially light up green.

   After a short period LED 3 will light up blue to indicate that an update is available and the following message will be displayed in the **Device Output** window.

   ```sh
   Remote debugging from host <host IP address>, port <host port>.
   INFO: Application starting
   INFO: Press SAMPLE_BUTTON_1 to allow the deferral.

   ```

1. Press button A to start the update.

   LED 2 will turn yellow to indicate that the update is being accepted and installed.

   After a few seconds, the Deferred Update application exits and the Blink/Hello World application starts.

   LED 2 and LED 3 will turn off as the Deferred Update application exits.

   LED 1 will blink red to indicate that the Blink/Hello World application was deployed.

## Troubleshooting

See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.


