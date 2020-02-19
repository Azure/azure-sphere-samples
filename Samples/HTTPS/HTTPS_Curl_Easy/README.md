# Sample: HTTPS_Curl_Easy

This sample C application demonstrates how to use the cURL "easy" API with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using curl](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample periodically downloads the index web page at example.com, by using cURL over a secure HTTPS connection.
It uses the cURL "easy" API, which is a synchronous (blocking) API.

You can also modify the sample to use mutual authentication if your website is configured to do so. Instructions on how to modify the sample are provided below; however, they require that you already have a website and certificates configured for mutual authentication. See [Connect to web services - mutual authentication](https://docs.microsoft.com/azure-sphere/app-development/curl#mutual-authentication) for information about configuring mutual authentication on Azure Sphere. For information about configuring a website with mutual authentication for testing purposes, you can use [Configure certificate authentication in ASP.NET Core](https://docs.microsoft.com/aspnet/core/security/authentication/certauth?view=aspnetcore-3.0).

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures Visual Studio to use CMake with the correct command-line options. |
|launch.vs.json |Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

The sample requires the following hardware:

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

- By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, follow the steps in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer,  andyour computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.0 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the HTTPS_Curl_Easy_HighLevelApp sample in the HTTPS folder.

## Set up hardware to display output

To prepare your hardware to display output from the sample, see "Set up hardware to display output" for [Windows](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) or [Linux](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output).

## Add host names to the application manifest

The sample can only connect to websites listed in the application manifest. In the "AllowedConnections" section of the [app_manifest.json](https://docs.microsoft.com/azure-sphere/app-development/app-manifest) file, add the host name of each website to which you want the sample to connect. For example, the following adds Contoso.com to the list of allowed websites.

```json
"Capabilities": {
    "AllowedConnections": [ "www.example.com", "www.Contoso.com"],
  },
```

## Downloading from another website

To download data from a website other than the default website:

1. If you haven't already done so, add the URL to the *AllowedConnections* capability of the application manifest.

2. Open main.c, and then go to the following statement.

    ```c
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, "https://example.com")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }
    ```

3. Change **example.com** to the new URL.

4. If the website is using SSL, add the trusted Root CA to the certs path.
     1. Put the trusted root CA cert in the certs/ folder (and optionally remove the existing DigiCert Global Root CA cert).
     1. Update line 12 of CMakeLists.txt to include the new trusted root CA cert in the imagepackage, instead of the DigiCert Global Root CA cert.
     1. Update line 159 of main.c to point to the new trusted root CA cert.

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build and deploy this sample:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

## Using the sample with mutual authentication

> [!NOTE]
> These instructions are for testing purposes only and should not be used in a production environment.

Continue here to modify and run the sample on a website that has mutual authentication configured.

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

1. Add [tlsutils](https://docs.microsoft.com/azure-sphere/app-development/baseapis#tls-utilities-library) to `TARGET_LINK_LIBRARIES` in CMakeLists.txt.

```c
TARGET_LINK_LIBRARIES(${PROJECT_NAME} applibs pthread gcc_s c curl tlsutils)
```

2. Open main.c and add the deviceauth_curl.h header file after curl.h.

```c
#include <curl/curl.h>
#include <tlsutils/deviceauth_curl.h>
```

### Add TLS utilities functions

Use the **DeviceAuth_CurlSslFunc** function or create a custom authentication function using **DeviceAuth_SslCtxFunc**. See [Connect to web services - mutual authentication](https://docs.microsoft.com/azure-sphere/app-development/curl#mutual-authentication) for more information about these functions.

#### To use DeviceAuth_CurlSslFunc

1. In main.c, add this code to the **PerformWebPageDownload** function after the **if** statement that sets **CURLOPT_VERBOSE**.

```c
    // Configure SSL to use device authentication-provided client certificates
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_CTX_FUNCTION, DeviceAuth_CurlSslFunc)) !=
        CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_SSL_CTX_FUNCTION", res);
        goto cleanupLabel;
    }
```

#### To create a custom authentication function using DeviceAuth_SslCtxFunc

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
