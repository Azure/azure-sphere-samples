# Sample: Deferred update

This sample demonstrates how to [defer updates](https://docs.microsoft.com/azure-sphere/app-development/device-update-deferral) to an Azure Sphere application. In this sample, you will do the following:

- Set up the Blink or Hello World application as an update for your device, so that it will eventually replace the Deferred Update application.
- Load the Deferred Update application onto the device.
- Use the device buttons to defer or accept the update.
- Observe that the Deferred Update application is replaced with the Blink/Hello World application when the update is accepted.

The sample uses the following Azure Sphere libraries and requires [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages buttons and LEDs on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages  during debugging. |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Monitors and dispatches events. |
| [sysevent](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-sysevent/sysevent-overview) | Interacts with system event notifications. |

## Prerequisites

The sample requires the following hardware:

- Azure Sphere device

   **Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

- Azure Sphere SDK version 20.07 or above. To check, run **azsphere show-version** at the command prompt.

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

Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the DeferredUpdate_HighLevelApp sample in the DeferredUpdate folder.

Build the app using your Azure Sphere development environment: Visual Studio, Visual Studio Code, or the Windows or Linux command line, as described in the following sections. 

### Build the app with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.

1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window. After you build the application, do not deploy it with Visual Studio. See [Troubleshooting samples](../../troubleshooting.md) if you encounter errors.

1. Run `azsphere device sideload deploy --imagepackage <deferred-update-imagepackage-name>` to sideload the application to the device.

1. Run `azsphere device image list-installed` to confirm that the Deferred Update application and GDB server are installed on the device.

### Build the app with Visual Studio Code

1. Start Visual Studio Code and open the sample application folder in the Azure Sphere samples repo. If Visual Studio Code displays a dialog box indicating that no CMake kits are available, select `Do not use a kit`.

1. When CMake cache generation is complete, press F5 to debug the project and then select Azure Sphere (GDB). If the project has not previously been built, or if files have changed and the project must be rebuilt, Visual Studio Code will build the project before debugging starts.

1. Wait several seconds for Visual Studio Code to compile the application, create an image package, deploy it to the board, and start it in debug mode. You'll see status updates along the way. First, CMake determines whether the application needs to be built; if so, focus shifts to the output window, which displays the output from CMake/Build.

   Next, the output window shows the output from **azsphere** as it deploys the image package to the device. Finally, the Debug Console receives focus and shows gdb output.

### Build the app on the Windows command line

1. Create or navigate to the directory that will contain the build .imagepackage files that will be generated during the build process. For example, to create and open a new directory called "buildfiles" you would enter the following commands at the command line.

   ```sh
   mkdir buildfiles
   cd buildfiles
   ```

1. Run `cmake` at the command line as shown below. Set the final parameter to the pathname of the directory that contains the source files for the DeferredUpdate_HighLevelApp sample application on your local machine.

    ```sh
    cmake ^
    -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereToolchain.cmake" ^
    -DAZURE_SPHERE_TARGET_API_SET="5" ^
    -DCMAKE_BUILD_TYPE="Debug" ^
    <path to DeferredUpdate_HighLevelApp sample>
    ```

1. Run ninja to build the application and create the image package file.

    `ninja`

### Build the app on the Linux command line

1. Create or navigate to the directory that will contain the files to be generated during the build process. For example, to create and open a new directory called `build`, you would enter the following commands at the command line.

   ```sh
   mkdir build
   cd build
   ```

1. Run `cmake` at the command line as shown below. First, make the following changes to the parameter list. Replace the last parameter with the path to the DeferredUpdate_HighLevelApp sample application on your local machine.

   ```sh
      cmake \
      -G "Ninja" \
      -DCMAKE_TOOLCHAIN_FILE="/opt/azurespheresdk/CMakeFiles/AzureSphereToolchain.cmake" \
      -DAZURE_SPHERE_TARGET_API_SET="5" \
      -DCMAKE_BUILD_TYPE="Debug" \
      <path to DeferredUpdate_HighLevelApp sample>  
   ```

1. Run ninja to build the application and create the imagepackage file.

    ```sh
    ninja
    ```

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

3. Press button A to accept the update. LED 2 will turn yellow to indicate that updates are being accepted. After a few seconds, the following update message is displayed, the Deferred Update application exits, and the Blink/Hello World application starts.

   ```
   INFO: Received update event: 2019-10-08 14:35:28
   INFO: Status: Final (2)
   INFO: Max deferral time: 0 minutes
   INFO: Update Type: Application (1).
   INFO: Final update. App will update in 10 seconds.

   INFO: Application exiting
   ```

## Troubleshooting

When you debug the Deferred Update application in Visual Studio or Visual Studio Code, the Blink/Hello World application might be deployed before you press button A. This can happen if the update occurs before the Deferred Update application has a chance to defer the update. To avoid the issue, ensure that you start debugging the Deferred Update application as soon as the device restarts so the application isn't prematurely deployed.

See [Troubleshooting samples](../../troubleshooting.md) if you encounter other errors.
