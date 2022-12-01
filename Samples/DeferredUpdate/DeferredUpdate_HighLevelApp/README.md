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

This sample demonstrates how to [defer updates](https://learn.microsoft.com/azure-sphere/app-development/device-update-deferral) to an Azure Sphere application. In this sample, you will do the following:

- Set up the Blink or Hello World application as an update for your device, so that it will eventually replace the Deferred Update application.
- Load the Deferred Update application onto the device.
- Use the device buttons to defer or accept the update.
- Observe that the Deferred Update application is replaced with the Blink/Hello World application when the update is accepted.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Monitors and dispatches events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons and LEDs on the device. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging. |
| [sysevent](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Interacts with system event notifications. |

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

This sample requires the following items:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- Azure Sphere SDK version 22.11 or above. To check, run [**azsphere show-version**](https://learn.microsoft.com/azure-sphere/reference/azsphere-show-version) at the command prompt.

## Setup

First, obtain the sample. Next, create the Blink application `.imagepackage` file and prepare the device to receive updates.

### Obtain the sample

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *DeferredUpdate* sample in the *DeferredUpdate* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/DeferredUpdate/).

### Create the Blink application .imagepackage file

1. Connect the Azure Sphere device to your computer through USB and ensure that the device is connected to the internet.

1. Open a [command-line interface](https://learn.microsoft.com/azure-sphere/reference/overview) using PowerShell, Windows command prompt, or Linux command shell.

1. Run [**azsphere device enable-development**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#enable-development). This command sideloads the application debugging capabilities and GDB server, and moves the device to the Development device group. This command doesn't delete the application.

1. Follow the tutorial to [build a high-level application](https://learn.microsoft.com/azure-sphere/install/qs-blink-application).

1. Confirm that the application is deployed and running (LED1 blinks red).

1. Note the location of the `Blink.imagepackage` file. For example: `<path to your Blink folder>\Blink\out\ARM-Debug\Blink.imagepackage`.

1. Delete the application from the device.

    1. Run [**azsphere device image list-installed**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#image-list-installed) to display the component ID of the application.
    1. Run the [**azsphere device sideload delete**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#sideload-delete) command to delete the application. For example:

        Azure Sphere CLI:

        ```
        azsphere device sideload delete --component-id <blink-component-id>
        ```

        Azure Sphere classic CLI:
        ```
        azsphere device sideload delete --componentid <blink-component-id>
        ```

### Prepare the device to receive updates

1. If you haven't already, run **azsphere product create --name MyProduct** at the command prompt. This command [creates a product](https://learn.microsoft.com/azure-sphere/reference/azsphere-product#create) and the [standard device groups](https://learn.microsoft.com/azure-sphere/deployment/deployment-concepts#device-groups).

1. Run the [**azsphere device update**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#update) command to move the device to the **Field Test** group. For example:

    Azure Sphere CLI:

    ```
    azsphere device update --device-group "MyProduct/Field Test"
    ```

    Azure Sphere classic CLI:

    ```
    azsphere device update --productname MyProduct --devicegroupname "Field Test"
    ```

1. Run the [**azsphere device wifi show-status**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#wifi-show-status) command to verify that the Azure Sphere device is connected to your WiFi network.

## Build and run the sample

Perform the following steps to build and run this sample:

1. [Build and run the Deferred Update application](#build-and-run-the-deferred-update-application).
1. [Deploy the Blink.imagepackage file](#deploy-the-blinkimagepackage-file).
1. [Update the device](#update-the-device).

### Build and run the Deferred Update application

To build and run the Deferred Update application, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

After the application starts up, LED2 will light up green to indicate that the Deferred Update application is running and updates will be deferred.

### Deploy the Blink.imagepackage file

When you deploy the imagepackage file a deployment is created for the **Field Test** device group. For example:

Azure Sphere CLI:

1. Upload the image package to your Azure Sphere tenant by using [**azsphere image add**](https://learn.microsoft.com/azure-sphere/reference/azsphere-image#add).

   ```
   azsphere image add --image <path to your Blink folder>\Blink\out\ARM-Debug\Blink.imagepackage>
   ```

2. Create a new deployment for a device group for the uploaded images using [**azsphere device-group deployment create**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device-group#deployment-create).

   ```
   azsphere device-group deployment create --device-group "MyProduct/Field Test" --images <image-ID>
   ```

Azure Sphere classic CLI:

Upload the image package to your Azure Sphere tenant and create a new deployment for a device group for the uploaded images using [**azsphere device-group deployment create**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device-group#deployment-create).

```
azsphere device-group deployment create --productname MyProduct --devicegroupname "Field Test" --filepath <path to your Blink folder>\Blink\out\ARM-Debug\Blink.imagepackage>
```

### Update the device

An Azure Sphere device is not automatically notified when updates are available. It checks for updates when it first connects to the internet and at regular intervals (currently 24 hours) thereafter. The device will also check for updates when it is restarted.

The sample uses the following LEDs:

| LED | Description |
|---------|---------|
| LED 3 | Blue if an update was received. |
| LED 2 | Green if updates are being deferred, yellow if updates are being accepted. |

You can update the device with the Deferred Update application running inside or outside the Visual Studio or Visual Studio Code integrated development environment (IDE), continuing with one of the options below.

**Outside an IDE**

To update the device with the Deferred Update application running outside an IDE:

1. After you deploy the Blink.imagepackage file, restart the device.

   On restart the device will check for updates.

   After restart, LED 2 will initially light up green.

   After a short period, LED 3 will light up blue to indicate that an update is available.

1. Press button A to start the update.

   LED 2 will turn yellow to indicate that the update is being accepted and installed.

   After a few seconds, LED 2 and LED 3 will turn off as the Deferred Update application exits.

   LED 1 will blink red to indicate that the Blink/Hello World application was deployed, and the Deferred Update application and GDB server were removed.

1. Run [**azsphere device image list-installed**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#image-list-installed) to verify that the Blink/Hello World application is installed, and the Deferred Update application and GDB server are no longer installed.

**Inside an IDE**

To update the device with the Deferred Update application running inside an IDE:

1. After you deploy the Blink.imagepackage, restart the device.

   On restart, the device will check for updates.

   After restart, LED 2 will initially light up green.

   After a short period, LED 3 will light up blue to indicate that an update is available and the following message will be displayed in the **Device Output** window:

   ```
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

## Next Steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
