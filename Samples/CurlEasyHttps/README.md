# Sample: CurlEasyHttps

This sample C application demonstrates how to use the cURL "easy" API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/en-us/azure-sphere/app-development/curl).

The sample periodically downloads the index web page at example.com, by using cURL over a secure HTTPS connection.
It uses the cURL "easy" API, which is a synchronous (blocking) API.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/install/overview).
1. Clone the Azure Sphere samples repo and then open the CurlEasyHttps sample from within your copy.
1. Connect your Azure Sphere device to your PC by USB.
1. Open an Azure Sphere Developer Command Prompt and enable Wi-Fi and application development on your Azure Sphere device, if you have not already done so.
1. In Visual Studio, open CurlEasyHttps.sln and press F5 to compile and build the solution and load it onto the device for debugging.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).