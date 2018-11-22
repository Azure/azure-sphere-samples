# Sample: Mutable storage

This sample C application illustrates how to use [storage](https://docs.microsoft.com/azure-sphere/app-development/storage) in an Azure Sphere application.

When you press button A, the sample opens a persistent data file on the device, increments the value in it, and closes the file. When you press button B, the sample deletes the file. The file persists if the application exits or is updated. However, if you delete the application by using the **azsphere device sideload delete** command, the file is deleted as well.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|---------|---------|
| gpio |  Enables use of buttons and LEDs |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Manages persistent user data |     

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 18.11 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MutableStorage sample.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/en-us/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging)Enable application development on your device if you have not already done so:

   `azsphere device prep-debug`
1. In Visual Studio, open MutableStorage.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. When the application starts, press button A to open and write to a file. Press the button repeatedly to increment the value in the file. Press button B to delete the file.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).