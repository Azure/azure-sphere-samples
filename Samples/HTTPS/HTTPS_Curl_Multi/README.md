# Sample: HTTPS_Curl_Multi

This sample C application demonstrates how to use the cURL 'multi' API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample downloads multiple web pages concurrently by using the cURL 'multi' interface. The content is output as soon as it arrives. Pressing button A on the MT3620 development board initiates the web transfers. After the sample validates the server identity, communication occurs over HTTP or HTTPS. At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi' 
interface is non-blocking.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`
1. In HTTPS_Curl_Multi/main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    int err = Networking_SetInterfaceState("eth0", true);
    if (err == -1) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```
1. In HTTPS_Curl_Multi/ui.c, ensure that the global constant `networkInterface` is set to "eth0". In source file HTTPS_Curl_Multi/ui.c, search for the following line:

   `static const char networkInterface[] = "wlan0";`

   Change this line to:

   `static const char networkInterface[] = "eth0";`


You can also configure a static IP address on an Ethernet or Wi-Fi interface. If you have configured a device with a static IP and require name resolution your application must set a static DNS address. For more information, see the topics *"Static IP address"* and *"Static DNS address"* in [Use network services](https://docs.microsoft.com/azure-sphere/network/use-network-services).


The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Enables digital input for button A |
|[log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages during debugging  |
|[storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview)    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |
|[networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets and sets network interface configuration |

## Contents

| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
|launch.vs.json |Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

The sample requires the following hardware:

* [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.07 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the HTTPS_Curl_Multi_HighLevelApp sample in the HTTPS folder.

## Add host names to the application manifest

The sample can only connect to websites listed in the application manifest. In the "AllowedConnections" section of the [app_manifest.json](https://docs.microsoft.com/azure-sphere/app-development/app-manifest) file, add the host name of each website to which you want the sample to connect. For example, the following adds Contoso.com to the list of allowed websites.

```json
"Capabilities": {
    "AllowedConnections": [ "www.example.com", "www.Contoso.com"],
  },
```

## Downloading from another website

To download data from a website other than the default website, you'll need to get the root CA certificate from the website, and modify the sample to use the new website and its certificate.

### Download the root CA certificate

If the website uses SSL, you may need to use a different root CA certificate. To download the certificate from the website, follow these instructions:

1. Open the browser and click the Secure icon, which is a padlock in the address bar.
1. Select Certificate.
1. Open the Certification Path tab.
1. Select the top certificate in the hierarchy and then select View Certificate.
1. Open the Details tab and select Copy to File.
1. In the Certificate Export Wizard, click Next.
1. Select the Base-64 encoded X.509 (.CER) format and then click Next.
1. Type the file name to which to export the certificate and then click Next.
1. Click Finish to complete the wizard.
1. Rename the downloaded certificate file to have the .pem extension.

### Modify the sample to use the new website

1. If you haven't already done so, add the hostname to the AllowedConnections capability of the application manifest.
1. Open web_client.c, go to the following statement, and change `https://httpstat.us/200?sleep=5000` and `https://httpstat.us/400?sleep=1000` to the URLs of the website you want to connect to.

    ```c
    // The web transfers executed with cURL.
    WebTransfer webTransfers[] = {
                // Download a web page with a delay of 5 seconds with status 200.
                {.url = "https://httpstat.us/200?sleep=5000", .easyHandle = NULL},
                // Download a web page with a delay of 1 second with status 400.
                {.url = "https://httpstat.us/400?sleep=1000", .easyHandle = NULL}};
    ```

1. Update the sample to use a different root CA certificate, if necessary: 
     1. Put the trusted root CA certificate in the certs/ folder (and optionally remove the existing bundle.pem certificate).
     1. Update line 16 of CMakeLists.txt to include the new trusted root CA certificate in the image package, instead of the bundle.pem certificate.
     1. Update line 195 of web_client.c to point to the new trusted root CA certificate.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## To start the download

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
