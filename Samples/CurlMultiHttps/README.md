# Sample: CurlMultiHttps

This sample C application demonstrates how to use the cURL "multi" API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample downloads multiple web pages concurrently by using the cURL 'multi' interface. The content is output as soon as it arrives. Pressing button A on the MT3620 development board initiates the web transfers. After the sample validates the server identity, communication occurs over HTTP or HTTPS. At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi' 
interface is non-blocking.

The sample uses [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta) and the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
| gpio | Enables digital input for button A |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.02 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the  Clone the Azure Sphere samples repo and then open the CurlMultiHttps sample from within your copy.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [Wi-Fi](https://docs.microsoft.com/azure-sphere/install/configure-wifi) and [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging) on your Azure Sphere device, if you have not already done so.
1. In Visual Studio, open CurlMultiHttps.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. Press button A on the board to start download.

The sample downloads status information for HTTP statuses 200 (success) and 400 (bad request) from the httpstat.us website.  

### Troubleshooting

If you see numerous errors in the Visual Studio Error List relating to missing headers and undefined identifiers, or if when building the app, you see the following error in the Visual Studio Build Output:

   `error MSB6004: The specified task executable location "C:\Program Files (x86)\Microsoft Azure Sphere SDK\\SysRoot\tools\gcc\arm-poky-linux-musleabi-gcc.exe" is invalid.`

Then it is likely you have an older version of the Azure Sphere SDK installed; ensure you have version 19.02 or newer.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
