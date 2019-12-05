# Sample: HelloWorld_HighLevelApp

This sample shows how to use CMake to build an Azure Sphere high-level application from the command line. This repository contains a sample CMake project that uses the same code as the [Azure Sphere Blink sample](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#build-and-run-the-blink-sample).

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [gpio.h](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) |Contains functions and types that interact with GPIOs.  |
| [log.h](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |

## Contents
| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures Visual Studio to use CMake with the correct command-line options. |
|launch.vs.json |Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

Before you continue, verify that:

- You have connected your Azure Sphere device to your PC
- You have completed all the steps to [install Azure Sphere](..\install\overview.md) for Windows
- You have [set up your development environment](development-environment-windows.md) for Windows
- You have cloned the entire samples repository locally (`git clone https://github.com/Azure/azure-sphere-samples.git`)

By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To set the target hardware for an application, set the CMakeSettings.json fields to use the corresponding hardware definition. You can use the definition files that are supplied with the samples or other files supplied by the hardware vendor.

**To target alternative hardware**

 Set the Target Hardware Definition Directory for your hardware.
 The Target Hardware Definition Directory identifies the folder that contains the hardware definition files for the target hardware. This path is relative to the workspace of the project.

   - Edit the **AzureSphereTargetHardwareDefinitionDirectory** field in the CMakeSettings.json file. For example:

      ```json
      "AzureSphereTargetHardwareDefinitionDirectory": "..\\..\\..\\Hardware\\seeed_mt3620_rdb",
      ```

The sample already specifies the required target hardware definition, sample_hardware.json. This file is supplied in the Hardware folder in the samples repository.

## Prepare your device for development and debugging

Before you can build a sample application on your Azure Sphere device or develop new applications for it, you must enable development and debugging. By default, Azure Sphere devices are "locked"; that is, they do not allow applications under development to be loaded from a PC, and they do not allow debugging of applications. Preparing the device for debugging removes this restriction.

The azsphere device **enable-development** command configures the device to accept applications from a PC for debugging and loads the debugging server onto the device. It also assigns the device to a [device group](../deployment/deployment-concepts.md#device-groups) that does not allow cloud-based application updates. During application development and debugging, you should leave the device in this group so that cloud-based application updates do not overwrite the application under development.

**To prep your device**

1. Make sure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.

1. In an Azure Sphere Developer Command Prompt window, type the following command:

   ```sh
   azsphere device enable-development
   ```

   You should see output similar to the following:

   ```sh
    Getting device capability configuration for application development.
    Downloading device capability configuration for device ID '<device ID>'.
    Successfully downloaded device capability configuration.
    Successfully wrote device capability configuration file 'C:\Users\user\AppData\Local\Temp\tmpD732.tmp'.
    Setting device group ID 'a6df7013-c7c2-4764-8424-00cbacb431e5' for device with ID '<device ID>'.
    Successfully disabled over-the-air updates.
    Enabling application development capability on attached device.
    Applying device capability configuration to device.
    Successfully applied device capability configuration to device.
    The device is rebooting.
    Installing debugging server to device.
    Deploying 'C:\Program Files (x86)\Microsoft Azure Sphere SDK\DebugTools\gdbserver.imagepackage' to the attached device.
    Image package 'C:\Program Files (x86)\Microsoft Azure Sphere SDK\DebugTools\gdbserver.imagepackage' has been deployed to the attached device.
    Application development capability enabled.
    Successfully set up device '<device ID>' for application development, and disabled over-the-air updates.
    Command completed successfully in 00:00:38.3299276.
   ```

The device remains enabled for debugging and closed to cloud-based application updates until you explicitly change it. To disable debugging and allow application updates, use the **enable-cloud-test** command.

## Build the sample

To create the build and .imagepackage files for the HelloWorld_HighLevelApp sample application do the following:

1. Create or navigate to the directory that will contain the files to be generated during the build process. For example, enter the following commands at the command line to create and open a directory called "build".

   ```sh
   mkdir build
   cd build
   ```

1. At the command line, in the directory that you opened in step 1, run cmake. For example:

    ```sh
    cmake ^
    -G "Ninja" ^
    -DCMAKE_INSTALL_PREFIX:PATH="." ^
    -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereToolchain.cmake" ^
    -DAZURE_SPHERE_TARGET_API_SET="3" ^
    -DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY="C:\AzSphere\azure-sphere-samples\Hardware\mt3620_rdb" ^
    -DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION="sample_hardware.json" ^
    --no-warn-unused-cli ^
    -DCMAKE_BUILD_TYPE="Debug" ^
    -DCMAKE_MAKE_PROGRAM="ninja.exe" ^
     C:\AzSphere\azure-sphere-samples\Samples\HelloWorld\HelloWorld_HighLevelApp
   ```

   You pass the following parameters to cmake:
   - *DCMAKE_TOOLCHAIN_FILE*  
          The location of the compiler tool chain file. In the example it is set to the default location of the toolchain file that is installed with the Azure Sphere SDK.

   - *DAZURE_SPHERE_TARGET_API_SET*  
          The desired Azure Sphere API set to use, such as "3+Beta1909". The value for this parameter is found in the Azure Sphere toolchain file. In the example it is set to 3.

   - *DCMAKE_BUILD_TYPE*  
          The build type. Possible values are Debug and Release.

   - *DCMAKE_MAKE_parameter*  
          The make program to use to build the application. In the example it is set to use ninja.

   - *DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY*  
          The location of the directory that contains the [hardware definition file](https://docs.microsoft.com/azure-sphere/hardware/hardware-abstraction) for your Azure Sphere device. 
          In the example it is set to the absolute pathname of the hardware definition directory for the MT3620 RDB. If you have cloned the Azure Sphere samples repository then it will 
          be in a sub-directory of the azure-sphere-samples directory. You will need to provide the path to the location of the azure-sphere-samples director on your local machine.

   - *DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION*  
          The name of the hardware definition file.

   - The final parameter is the pathname to the directory containing the source files for the sample.

1. Run ninja to build the application and create the imagepackage file.

```sh
   ninja
```

## Run the sample

1. Open an Azure Sphere Developer Command Prompt.
1. Open the directory from which you ran cmake and ninja and now contains the image package. 
1. Run the **azsphere device sideload deploy** command:

    `azsphere.exe device sideload deploy --imagepackage HelloWorld_HighLevelApp.imagepackage`

   
An LED ( LED 1 on the MT3620 RDB) will begin to blink.

## License
For license details, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
