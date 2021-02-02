---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – HTTPS cURL Easy
urlFragment: HTTPS-cURL-Easy
extendedZipContent:
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to use the cURL Easy interface with Azure Sphere over a secure HTTPS connection."
---

# Sample: HTTPS cURL Easy

This sample demonstrates how to use the cURL Easy interface with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using cURL](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample periodically downloads the index web page at example.com, by using cURL over a secure HTTPS connection.
It uses the cURL Easy interface, which is a synchronous (blocking) API.

The sample logs the downloaded content. If the size of the downloaded content exceeds 2 KiB, the sample pauses the download, prints the content that has been downloaded so far, and then resumes the download. Refer to cURL `curl_easy_pause` API for more information.

You can modify the sample to use mutual authentication if your website is configured to do so. Instructions on how to modify the sample are provided below; however, they require that you already have a website and certificates configured for mutual authentication. See [Connect to web services - mutual authentication](https://docs.microsoft.com/azure-sphere/app-development/curl#mutual-authentication) for information about configuring mutual authentication on Azure Sphere. For information about configuring a website with mutual authentication for testing purposes, you can use [Configure certificate authentication in ASP.NET Core](https://docs.microsoft.com/aspnet/core/security/authentication/certauth?view=aspnetcore-3.0).

You can configure a static IP address on an Ethernet or Wi-Fi interface. If you have configured a device with a static IP and require name resolution your application must set a static DNS address. For more information, see the topics *"Static IP address"* and *"Static DNS address"* in [Use network services](https://docs.microsoft.com/azure-sphere/network/use-network-services).

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|[log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages in the Device Output window during debugging  |
|[storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview)    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets and sets network interface configuration |

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

     **Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`
1. In HTTPS_Curl_Easy/main.c, ensure that the global constant `networkInterface` is set to "eth0". In source file HTTPS_Curl_Easy/main.c, search for the following line:

    `static const char networkInterface[] = "wlan0";`

    Change this line to:

    `static const char networkInterface[] = "eth0";`
1. In HTTPS_Curl_Easy/main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

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

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the HTTPS_Curl_Easy sample in the HTTPS folder.

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

1. Open main.c, go to the following statement, and change `example.com` to the URL of the website you want to connect to.

    ```c
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, "https://example.com")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }
    ```

1. Update the sample to use a different root CA certificate, if necessary: 
     1. Put the trusted root CA certificate in the certs/ folder (and optionally remove the existing DigiCert Global Root CA certificate).
     1. Update line 14 of CMakeLists.txt to include the new trusted root CA certificate in the image package, instead of the DigiCert Global Root CA certificate.
     1. Update line 185 of main.c to point to the new trusted root CA certificate.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Using the sample with mutual authentication

**Note:**
These instructions are for testing purposes only and should not be used in a production environment.

Continue here to modify and run the sample on a website that is configured to require mutual authentication.

### Update the application manifest

Add the following to app_manifest.json.

1. Add the `DeviceAuthentication` capability, and use it to specify your tenant ID.

1. If you haven't already done so, in the `AllowedConnections` capability, add the HTTPS endpoint of the website that has mutual authentication configured.

The following code snippet is an example entry.

```json
"Capabilities": {
    "AllowedConnections" : [ "www.example.com" ],
    "DeviceAuthentication": "77304f1f-9530-4157-8598-30bc1f3d66f0"
  },
```

### Add the TLS utilities library

1. Add [tlsutils](https://docs.microsoft.com/azure-sphere/app-development/baseapis#tls-utilities-library) to `target_link_libraries` in CMakeLists.txt.

```c
target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl tlsutils)
```

2. Open main.c and add the deviceauth_curl.h header file after curl.h.

```c
#include <curl/curl.h>
#include <tlsutils/deviceauth_curl.h>
```

### Add TLS utilities functions

Use the [**DeviceAuth_CurlSslFunc**](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/tlsutils/function-deviceauth-curlsslfunc) function or create a custom authentication function using [**DeviceAuth_SslCtxFunc**](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/tlsutils/function-deviceauth-sslctxfunc). See [Connect to web services - mutual authentication](https://docs.microsoft.com/azure-sphere/app-development/curl#mutual-authentication) for more information about these functions.

#### Use DeviceAuth_CurlSslFunc

1. In main.c, add this code to the **PerformWebPageDownload** function after the **if** statement that sets **CURLOPT_VERBOSE**.

```c
    // Configure SSL to use device authentication-provided client certificates
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_CTX_FUNCTION, DeviceAuth_CurlSslFunc)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_SSL_CTX_FUNCTION", res);
        goto cleanupLabel;
    }
```

#### Create a custom authentication function using DeviceAuth_SslCtxFunc

1. Create your custom callback function using **DeviceAuth_SslCtxFunc** to perform the authentication. for example:

```c
    static CURLcode UserSslCtxFunction(CURL* curlHandle, void* sslCtx, void* userCtx)
    {
        DeviceAuthSslResult result = DeviceAuth_SslCtxFunc(sslCtx);

        if (result != DeviceAuthSslResult_Success) {
            Log_Debug("Failed to set up device auth client certificates: %d\n", result);
            return CURLE_SSL_CERTPROBLEM;
        }

        return CURLE_OK;
    }
```

2. In main.c, add your custom function above the **PerformWebPageDownload** function.

3. Add the following code to the **PerformWebPageDownload** function after the **if** statement that sets **CURLOPT_VERBOSE**.

```c
    // Configure SSL to use device authentication-provided client certificates
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_CTX_FUNCTION, UserSslCtxFunction)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_SSL_CTX_FUNCTION", res);
        goto cleanupLabel;
    }
```

## Troubleshooting

The following message in device output may indicate an out of memory issue:  

   `Child terminated with signal = 0x9 (SIGKILL)`

Currently, the Azure Sphere OS has a bug that causes a slow memory leak when using cURL and HTTPS. This slow memory leak can result in your application running out of memory. We plan to fix this bug in an upcoming quality release, and will announce it in the [IoT blog](https://techcommunity.microsoft.com/t5/internet-of-things/bg-p/IoTBlog) when it is available. 

Until the updated OS is released, you can mitigate this problem. However, the mitigation might degrade performance, so you should remove it as soon as the updated OS is available. To mitigate the problem, disable the CURLOPT_SSL_SESSIONID_CACHE option when you create cURL handles, as shown in the following example: 

`curl_easy_setopt(curlHandle, CURLOPT_SSL_SESSIONID_CACHE, 0);`

For details about how to set this option, see [CURLOPT_SSL_SESSIONID_CACHE explained](https://curl.haxx.se/libcurl/c/CURLOPT_SSL_SESSIONID_CACHE.html) in the cURL documentation. 