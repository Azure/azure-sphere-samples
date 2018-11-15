# Sample: CurlMultiHttps

This sample C application demonstrates how to use the cURL "multi" API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/en-us/azure-sphere/app-development/curl).

The sample downloads multiple web pages concurrently by using the cURL 'multi' interface. The content is output as soon as it arrives. Pressing button A on the MT3620 development board initiates the web transfers. After the sample validates the server identity, communication occurs over HTTP or HTTPS. At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi' 
interface is non-blocking.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|---------|---------|
| gpio | Enables digital input for button A |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/install/overview).
1. Clone the  Clone the Azure Sphere samples repo and then open the CurlMultiHttps sample from within your copy.
1. Connect your Azure Sphere device to your PC by USB.
1. Open an Azure Sphere Developer Command Prompt and enable Wi-Fi and application development on your Azure Sphere device, if you have not already done so.
1. In Visual Studio, open CurlMultiHttps.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. Press button A on the board to start download.

The sample downloads status information for HTTP statuses 200 (success) and 400 (bad request) from the httpstat.us website.  

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).