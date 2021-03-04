---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – WolfSSL API
urlFragment: WolfSSL
extendedZipContent:
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to use the wolfSSL API with Azure Sphere to download a web page over HTTPS."
---

# Sample: WolfSSL high-level app

This sample application demonstrates how to use the wolfSSL API with Azure Sphere to download a
web page over HTTPS. For details about using the wolfSSL library with Azure Sphere, see
https://docs.microsoft.com/azure-sphere/app-development/wolfssl-tls.

The sample has four parts.
1. It connects to example.com:443 (HTTPS) using a Linux AF_INET socket.
1. It uses wolfSSL to perform the TLS handshake.
1. It sends an HTTP GET request to retrieve a web page.
1. It reads the HTTP response and prints it to the console.

Each of the above stages is handled asynchronously.

In this sample, wolfSSL is used to perform the TLS handshake. TLS supports server name indication (SNI), where a server can host multiple websites. To perform a TLS handshake with a server which uses SNI, call wolfSSL_CTX_UseSNI after allocating the context with wolfSSL_CTX_new. For more information, see [Using Server Name Indication (SNI) with wolfSSL](https://www.wolfssl.com/using-server-name-indication-sni-with-wolfssl/).

You can readily replace parts 3 and 4 of the sample to use wolfSSL to perform the TLS handshake before connecting with a custom server or use a different protocol than HTTP.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|[log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages in the Device Output window during debugging  |
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets connectivity status |
|[storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview)    | Gets the path to the certificate file that is used to authenticate the server      |
| [wolfssl](https://docs.microsoft.com/azure-sphere/app-development/wolfssl-tls) | Handles the SSL handshake. |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events. |

## Contents

| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json | Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures Visual Studio to use CMake with the correct command-line options. |
| launch.vs.json | Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
| .vscode | Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

The sample requires the following hardware:

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`
1. In WolfSSL_HighLevelApp/main.c, ensure that the global constant `networkInterface` is set to "eth0". In the source file WolfSSL_HighLevelApp/main.c, search for the following line:

    `static const char networkInterface[] = "wlan0";`

    Change this line to:

    `static const char networkInterface[] = "eth0";`
1. In WolfSSL_HighLevelApp/main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    int err = Networking_SetInterfaceState(networkInterface, true);
    if (err == -1) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.01 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *WolfSSL_HighLevelApp* sample in the *WolfSSL* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/wolfssl/).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../..//BUILD_INSTRUCTIONS.md). If an error occurs, use the [wolfSSL manual](https://www.wolfssl.com/docs/wolfssl-manual/) to interpret error codes.


## Download from another website

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

1. The sample can connect only to host names or IP addresses listed in the application manifest. To add the hostname, in the "AllowedConnections" section of the [app_manifest.json](https://docs.microsoft.com/azure-sphere/app-development/app-manifest) file, add the host name of each website to which you want the sample to connect. For example, the following adds "example.com " and "Contoso.com " to the list of allowed websites.

   Note that sometimes there may be more than one hostname for the same website. For example, a website may have the hostnames "www.contoso.com " and "contoso.com " (both hostnames will resolve to the same IP address). In this case, you must choose which hostname to use and use it consistently throughout your code.

   ```json
   "Capabilities": {
    "AllowedConnections": [ "example.com", "Contoso.com"]
    },
    ```

1. Open main.c, go to the following statement, and change `SERVER_NAME` to the hostname of the website you want to connect to, `PORT_NUM` to the port number and certPath[] to the certificate path. Ensure that your server name and "AllowedConnections" hostname are identical. For example, if you specified "contoso.com " in "AllowedConnections", you must also specify "contoso.com " (not "www.contoso.com ") for the server name.

    ```c
    #define SERVER_NAME "example.com"
    static const uint16_t PORT_NUM = 443;
    static const char certPath[] = "certs/DigiCertGlobalRootCA.pem";
    ```

1. Update the sample to use a different root CA certificate, if necessary: 
     1. Put the trusted root CA certificate in the certs/ folder (and optionally remove the existing DigiCert Global Root CA certificate).
     1. Update CMakeLists.txt to include the new trusted root CA certificate in the image package, instead of the DigiCert Global Root CA certificate.
        Pass the new the certificate file name to `azsphere_target_add_image_package`.
     1. In main.c, pass the certificate file name to `Storage_GetAbsolutePathInImagePackage`.

1. If the website uses TLS version 1.2, change the "WOLFSSL_METHOD" to "wolfTLSv1_2_client_method()". Exit code 16 may indicate the that the TLS version is incorrect.

## Use wolfSSL with a different protocol

To use a protocol other than HTTP, replace the `WriteData` and `ReadData` functions, which send
the HTTP request and read the response, with the appropriate logic for another protocol.

