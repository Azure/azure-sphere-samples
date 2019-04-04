# Sample: CurlEasyHttps

This sample C application demonstrates how to use the cURL "easy" API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample periodically downloads the index web page at example.com, by using cURL over a secure HTTPS connection.
It uses the cURL "easy" API, which is a synchronous (blocking) API.

The sample uses [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta) and the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.02 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the Azure Sphere samples repo and then open the CurlEasyHttps sample from within your copy.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [Wi-Fi](https://docs.microsoft.com/azure-sphere/install/configure-wifi) and [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging) on your Azure Sphere device, if you have not already done so.
1. In Visual Studio, open CurlEasyHttps.sln and press F5 to compile and build the solution and load it onto the device for debugging.

### Troubleshooting the Azure Sphere app

If an error similar to the following appears in the Visual Studio Build output when you build the Azure Sphere app, you probably have an outdated version of the Azure Sphere SDK:

   `mt3620_rdb.h:9:10: fatal error: soc/mt3620_i2cs.h: No such file or directory`

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
