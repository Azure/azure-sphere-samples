# Build a sample application

The sample applications are designed to be built with Visual Studio on Windows, and with Visual Studio Code or the command-line interface (CLI) on both Windows and Linux, unless otherwise noted.

- [Build and run a high-level sample with Visual Studio](#build-and-run-a-sample-application-with-visual-studio)
- [Build and run a high-level sample with Visual Studio Code](#build-and-run-a-sample-application-with-visual-studio-code)
- [Build and run a high-level sample with the CLI](https://learn.microsoft.com/azure-sphere/install/qs-blink-application?pivots=cli)

To build and run real-time capable apps (RTApps), see [Tutorial: Build a real-time capable application](https://learn.microsoft.com/azure-sphere/install/qs-real-time-application) in the online documentation.

## Build and run a high-level sample application with Visual Studio

1. Install Visual Studio and the Azure Sphere Extension for Visual Studio, and any additional tools as described in the [online documentation](https://learn.microsoft.com/azure-sphere/install/install-sdk?pivots=visual-studio).

1. Open Visual Studio and select Open a local folder. Navigate to the folder that contains the sample you want to build. If the folder contains samples for both high-level apps and real-time capable apps (RTApps), make sure you open the correct folder.

1. If the application targets hardware-specific features, open the CMakeLists.txt file and change the TARGET_DIRECTORY setting to specify the folder that contains definitions for the hardware you're using. By default, the TARGET_DIRECTORY specifies HardwareDefinitions/mt3620_rbd, which matches the Seeed Azure Sphere MT3620 Development Kit:

   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")`

      Several hardware definitions are provided with the Azure Sphere SDK. For example, if you're using a SEEED MT3620 Mini Dev Board, specify HardwareDefinitions/seeed_mt3620_mdb instead.

1. In Visual Studio, select **View** > **Output** to display the **Output** window.
1. Ensure that your device is connected to your PC by USB.
1. In the Set startup item, select **<Project name> (HLCore)** to build a high-level app.
1. Press **F5** to build and deploy the application.

## Build and run a high-level sample application with Visual Studio Code

1. Install Visual Studio Code, the Azure Sphere Extension for Visual Studio Code, and any additional tools as described in the online documentation:

   - [for Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk?pivots=vs-code)
   - [for Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux?pivots=vs-code-linux)

1. Start Visual Studio Code. Select **File** > **Open Folder**, and then select the folder that contains the sample you want to build. If the folder contains samples for both high-level apps and real-time capable apps (RTApps), make sure you open the correct folder.

1. If the application targets hardware-specific features, open the CMakeLists.txt file and change the TARGET_DIRECTORY setting to specify the folder that contains definitions for the hardware you're using. By default, the TARGET_DIRECTORY specifies HardwareDefinitions/mt3620_rbd, which matches the Seeed Azure Sphere MT3620 Development Kit:

   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")`

   Several hardware definitions are provided with the Azure Sphere SDK. For example, if you're using a SEEED MT3620 Mini Dev Board, specify HardwareDefinitions/seeed_mt3620_mdb instead.

1. Press F5 to build and debug the project. If the project has not previously been built, or if files have changed and rebuilding is required, Visual Studio Code will build the project before debugging starts.

1. Wait several seconds for Visual Studio Code to build the application, create an image package, deploy it to the board, and start it in debug mode. You'll see status updates in the **Output** pane along the way.
