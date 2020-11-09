# Sample: Deferred update

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

- Azure Sphere SDK version 20.10 or above. To check, run **azsphere show-version** at the command prompt.

## Prepare the device to receive updates

1. Connect the Azure Sphere device to your computer through USB and ensure that the device is connected to the internet.

1. If you haven't already, run `azsphere product create --name MyProduct` at the command prompt. This command creates a product and the [standard device groups](https://docs.microsoft.com/azure-sphere/deployment/deployment-concepts#device-groups).

1. Follow the tutorial to [build a high-level application](https://docs.microsoft.com/azure-sphere/install/qs-blink-application).

1. Run `azsphere device-group deployment create --productname MyProduct --devicegroupname "Field Test" --filepath <blink-imagepackage-name>` to create a deployment for the Field Test device group.

1. Run `azsphere device enable-cloud-test --productname MyProduct --devicegroupname "Field Test"`. This command puts the device in the Field Test device group of the MyProduct product, so it can download the application when the device restarts.

1. Restart the device and confirm that the application is deployed and that LED1 blinks green.

1. Run `azsphere device enable-development`. This command sideloads the application debugging capabilities and GDB server, and moves the device to the Development device group. This command doesn't delete the application.

1. Run `azsphere device image list-installed` to display the component ID of the application.

1. Run `azsphere device sideload delete --componentid <blink-component-id>` to delete the application.

1. Run `azsphere device update --productname MyProduct --devicegroupname "Field Test"` to move the device back to the Field Test group. Don't restart the device. If the device restarts it will load the application from the cloud, so don't restart the device yet.

## Build the Deferred Update application

To build and deploy this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Run the Deferred Update application

You can run the Deferred Update application inside or outside the Visual Studio or Visual Studio Code integrated development environment (IDE), continuing with one of the options below. The sample uses the following LEDs.

| LED | Description |
|---------|---------|
| LED 3 | Blue if an update was received. |
| LED 2 | Green if updates are being deferred, yellow if updates are being accepted. |

### Run the sample outside an IDE

1. After you deploy the Deferred Update application, restart the device. LED 2 will light up green to indicate that updates are being deferred. After a short period, LED 3 will light up blue to indicate that an update was received.

1. Press button A to start the update. LED 2 will turn yellow to indicate that updates are being accepted. After a few seconds, LED 2 and LED 3 will turn off as the Deferred Update application exits. LED 1 will blink green to indicate that the Blink/Hello World application was deployed, and the Deferred Update application and GDB server were removed.

1. Run `azsphere device image list-installed` to verify that the Blink/Hello World application is installed, and the Deferred Update application and GDB server are no longer installed.

### Run the sample in an IDE

1. After you deploy the Deferred Update application, restart the device.

2. If it's not already open, quickly open the Deferred Update application in the IDE and then press F5 to start debugging. You must open the application in the IDE and start debugging before the device receives the Blink/Hello World application from the cloud. LED 3 will turn blue to indicate that the update is available, and the following message will be repeated in the **Device Output** window every minute while the application defers the pending update.

   ```
   INFO: Received update event: 2020-07-17 21:02:54
   INFO: Status: Pending (1)
   INFO: Max deferral time: 10020 minutes
   INFO: Update Type: Application (1).
   INFO: Deferring update for 1 minute.

   INFO: Received update event: 2020-07-17 21:02:54
   INFO: Status: Deferred (3)
   INFO: Max deferral time: 10020 minutes
   INFO: Update Type: Application (1).
   INFO: Update deferred.
   ```

3. Press button A to accept the update. LED 2 will turn yellow to indicate that updates are being accepted. After a few seconds, the following update message is displayed, the Deferred Update application exits, and the Blink/Hello World application starts.

   ```
   INFO: Received update event: 2020-07-17 21:05:11
   INFO: Status: Final (2)
   INFO: Max deferral time: 0 minutes
   INFO: Update Type: Application (1).
   INFO: Final update. App will update in 10 seconds.

   INFO: Application exiting
   ```

## Troubleshooting

When you debug the Deferred Update application in Visual Studio or Visual Studio Code, the Blink/Hello World application might be deployed before you press button A. This can happen if the update occurs before the Deferred Update application has a chance to defer the update. To avoid the issue, ensure that you start debugging the Deferred Update application as soon as the device restarts so the application isn't prematurely deployed.

See [Troubleshooting samples](../../troubleshooting.md) if you encounter other errors.
