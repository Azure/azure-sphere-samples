# Sample: Deferred update

This sample demonstrates how to [defer updates](https://docs.microsoft.com/azure-sphere/app-development/device-update-deferral) to an Azure Sphere application. In this sample, you will do the following:

- Set up the Blink application as an update for your device, so that it will eventually replace the Deferred Update application.
- Load the Deferred Update application onto the device.
- Use the device buttons to defer or accept the update.
- Observe that the Deferred Update application is replaced with the Blink application when the update is accepted.

The sample uses the following Azure Sphere libraries and requires [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons and LEDs on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging. |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Monitors and dispatches events. |
| [sysevent](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Interacts with system event notifications. |

## Prerequisites

The sample requires the following hardware:

- Azure Sphere device

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

- Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.

## Prepare the device to receive updates

1. Connect the Azure Sphere device to your PC through USB and ensure that the device is connected to the internet.

1. Open an Azure Sphere command prompt.

1. If you haven't already, run `azsphere product create --name MyProduct`. This command creates a product and the standard device groups: *Development*, *Field Test*, and *Production*.

1. Build the Blink application as described in [Quickstart: Build the Blink application](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application).

1. Run `azsphere device-group deployment create --productname MyProduct --devicegroupname "Field Test" --filepath <blink-imagepackage-name>` to create a deployment for the *Field Test* device group.

1. Run `azsphere device enable-cloud-test --productname MyProduct --devicegroupname "Field Test"`. This command puts the device in the *Field Test* device group of the "MyProduct" product, so it can download the Blink application when the device restarts.

1. Restart the device and confirm that the Blink application is deployed and that LED1 blinks green.

1. Run `azsphere device enable-development`. This command sideloads the application debugging capabilities and GDB server, and moves the device to the *Development* device group. This command doesn't delete the Blink application.

1. Run `azsphere device image list-installed` to display the component ID of the Blink application.

1. Run `azsphere device sideload delete --componentid <blink-component-id>` to delete the Blink application.

1. Run `azsphere device update --productname MyProduct --devicegroupname "Field Test"` to move the device back to the *Field Test* group. Don't restart the device. If the device restarts it will load the Blink application from the cloud, so don't restart the device yet.

## Load the Deferred Update application

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the DeferredUpdate_HighLevelApp sample in the DeferredUpdate folder.

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.

1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window. After you build the application, do not deploy it with Visual Studio. See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.

1. Run `azsphere device sideload deploy --imagepackage <deferred-update-imagepackage-name>` to sideload the application to the device.

1. Run `azsphere device image list-installed` to confirm that the Deferred Update application and GDB server are installed on the device.

## Run the Deferred Update application

You can run the Deferred Update application in or outside Visual Studio by continuing with one of the options below. The sample uses the following LEDs.

| LED | Description |
|---------|---------|
| LED 3 | Blue if an update was received. |
| LED 2 | Green if updates are being deferred, yellow if updates are being accepted. |

### Running the sample outside Visual Studio

1. After you deploy the Deferred Update application, restart the device. LED 2 will light up green to indicate that updates are being deferred. After a short period, LED 3 will light up blue to indicate that an update was received.

1. Press button A to start the update. LED 2 will turn yellow to indicate that updates are being accepted. After a few seconds, LED 2 and LED 3 will turn off as the Deferred Update application exits. LED 1 will blink green to indicate that the Blink application was deployed, and the Deferred Update application and GDB server were removed.

1. Run `azsphere device image list-installed` to verify that the Blink application is installed, and the Deferred Update application and GDB server are no longer installed.

### Running the sample in Visual Studio

1. After you deploy the Deferred Update application, restart the device.

2. If it's not already open, quickly open the Deferred Update application in Visual Studio and then press F5 to start debugging. You must open the application in Visual Studio and start debugging before the device receives the Blink application from the cloud. LED 3 will turn blue to indicate that the update is available, and the following message will be repeated in the **Device Output** window every minute while the application defers the pending update.

```console
INFO: Received update event: 2019-10-08 14:32:18
INFO: Status: Pending (1)
INFO: Max deferral time: 10019 minutes
INFO: Update Type: Application (1).
INFO: Deferring update for 1 minute.

INFO: Received update event: 2019-10-08 14:32:18
INFO: Status: Rejected (3)
INFO: Max deferral time: 10019 minutes
INFO: Update Type: Application (1).
INFO: Update rejected (update has been deferred).
```

3. Press button A to accept the update. LED 2 will turn yellow to indicate that updates are being accepted. After a few seconds, the following update message is displayed, the Deferred Update application exits, and the Blink application starts.

```console
INFO: Received update event: 2019-10-08 14:35:28
INFO: Status: Final (2)
INFO: Max deferral time: 0 minutes
INFO: Update Type: Application (1).
INFO: Final update. App will update in 10 seconds.

INFO: Application exiting
```

### Troubleshooting

When debugging the Deferred Update application in Visual Studio, the Blink application might be deployed without pressing button A. This can happen if the update is performed before the Deferred Update application has a chance to defer the update. To avoid the issue, ensure that you start debugging the Deferred Update application as soon as the device restarts so the Blink application isn't prematurely deployed.

See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.
