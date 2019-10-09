# Sample: HTTPS_Curl_Multi

This sample C application demonstrates how to use the cURL 'multi' API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample downloads multiple web pages concurrently by using the cURL 'multi' interface. The content is output as soon as it arrives. Pressing button A on the MT3620 development board initiates the web transfers. After the sample validates the server identity, communication occurs over HTTP or HTTPS. At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi' 
interface is non-blocking.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../Hardware/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:
   `"NetworkConfig" : true`
1. In main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    err = Networking_SetInterfaceState("eth0", true);
    if (err < 0) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

1. In the Project Properties, ensure that the Target API Set is 3+Beta1909.

The sample uses [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta) and the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
| gpio | Enables digital input for button A |
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## To build and run the sample

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md). 

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Clone the Azure Sphere samples repo and then open the HTTPS_Curl_Multi sample from within your copy.
1. Connect your Azure Sphere device to your PC by USB.
1. Connect to the Internet and enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging) on your Azure Sphere device, if you have not already done so.
1. In Visual Studio, open HTTPS_Curl_Multi.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. Press button A on the board to start download.

The sample downloads status information for HTTP statuses 200 (success) and 400 (bad request) from the httpstat.us website.  

The sample can only connect to websites listed in the application manifest. In the "AllowedConnections" section of the app_manifest.json file, add the host name of each website to which you want the sample to connect. For example, the following adds Contoso.com to the list of allowed websites.

```json
"Capabilities": {
    "AllowedConnections": ["httpstat.us", "Contoso.com"],
     .
     .
     .
  }
```

### Troubleshooting the Azure Sphere app

- Visual Studio returns the following error if the application fails to compile:

   `1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

   This error may occur for many reasons. Most often, the reason is that you did not clone the entire Azure Sphere Samples repository from GitHub. The samples depend on the hardware definition files that are supplied in the Hardware folder of the repository.

### To get detailed error information

By default, Visual Studio may only open the Error List panel, so that you see error messages like this:

`1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

To get more information, open the Build Output window. To open the window, select **View->Output**, then choose **Build** on the drop-down menu. The Build menu shows additional detail, for example:

```
1>------ Rebuild All started: Project: AzureIoT, Configuration: Debug ARM ------
1>main.c:36:10: fatal error: hw/sample_hardware.h: No such file or directory
1> #include <hw/sample_hardware.h>
1>          ^~~~~~~~~~~~~~~~~~~~~~
1>compilation terminated.
1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.
1>Done building project "AzureIoT.vcxproj" -- FAILED.
========== Rebuild All: 0 succeeded, 1 failed, 0 skipped ==========
```

In this case, the error is that hardware definition files aren't available.

The **Tools -> Options -> Projects and Solutions -> Build and Run** panel provides further controls for build verbosity.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
