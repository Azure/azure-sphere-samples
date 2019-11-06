# Sample: Mutable storage

This sample C application illustrates how to use [storage](https://docs.microsoft.com/azure-sphere/app-development/storage) in an Azure Sphere application.

When you press button A, the sample opens a persistent data file on the device, increments the value in it, and closes the file. When you press button B, the sample deletes the file. The file persists if the application exits or is updated. However, if you delete the application by using the **azsphere device sideload delete** command, the file is deleted as well.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
| gpio |  Enables use of buttons and LEDs |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Manages persistent user data | 

## To prepare the sample

**Note:**: By default, this sample targets MT3620 reference development board (RDB)  hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the README file in the Hardware folder.

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MutableStorage sample.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging) on your device if you have not already done so:

   `azsphere device enable-development`

## To build and run the sample

### Building and running the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

### Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Update the data file

1. When the application starts, press button A to open and write to a file. Press the button repeatedly to increment the value in the file. Press button B to delete the file.
