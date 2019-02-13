# Sample: Mutable storage

This sample C application illustrates how to use [storage](https://docs.microsoft.com/azure-sphere/app-development/storage) in an Azure Sphere application.

When you press button A, the sample opens a persistent data file on the device, increments the value in it, and closes the file. When you press button B, the sample deletes the file. The file persists if the application exits or is updated. However, if you delete the application by using the **azsphere device sideload delete** command, the file is deleted as well.

The sample uses the following Azure Sphere libraries and requires [Beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

|Library   |Purpose  |
|---------|---------|
| gpio |  Enables use of buttons and LEDs |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Manages persistent user data | 

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.02 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MutableStorage sample.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging) on your device if you have not already done so:

   `azsphere device prep-debug`
1. In Visual Studio, open MutableStorage.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. When the application starts, press button A to open and write to a file. Press the button repeatedly to increment the value in the file. Press button B to delete the file.

### Troubleshooting

If you see numerous errors in the Visual Studio Error List relating to missing headers and undefined identifiers, or if when building the app, you see the following error in the Visual Studio Build Output:

   `error MSB6004: The specified task executable location "C:\Program Files (x86)\Microsoft Azure Sphere SDK\\SysRoot\tools\gcc\arm-poky-linux-musleabi-gcc.exe" is invalid.`

Then it is likely you have an older version of the Azure Sphere SDK installed; ensure you have version 19.02 or newer.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
