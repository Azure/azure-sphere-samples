---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – HTTPS cURL Multi
urlFragment: HTTPS-cURL-Multi
extendedZipContent:
- path: HardwareDefinitions
  target: HardwareDefinitions
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to use the cURL Multi interface with Azure Sphere over a secure HTTPS connection."
---

# Sample: HTTPS cURL Multi

This sample demonstrates how to use the cURL Multi interface with Azure Sphere over a secure HTTPS connection. For details about using the libcurl library with Azure Sphere, see [Connect to web services using cURL](https://docs.microsoft.com/azure-sphere/app-development/curl).

The sample downloads multiple web pages concurrently by using the cURL Multi interface. The content is output as soon as it arrives. Pressing button A on the MT3620 development board initiates the web transfers. After the sample validates the server identity, communication occurs over HTTP or HTTPS. At the same time, LED1 blinks at a constant rate, demonstrating that the cURL Multi interface is non-blocking.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [curl](https://docs.microsoft.com/azure-sphere/reference/baseapis#curl-library) | Configures the data transfer and downloads the web page over HTTP/HTTPS. |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Enables digital input for button A. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)    | Displays messages in the Device Output window during debugging. |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets and sets network interface configuration. |
| [storage](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-storage/storage-overview)    | Gets the path to the certificate file that is used to authenticate the server. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

The sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

## Setup

Complete the following steps to set up this sample.

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.07 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *HTTPS_Curl_Multi* sample in the *HTTPS* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/https-curl-multi/).

1. Note that the sample can connect only to websites listed in the **AllowedConnections** capability of the [app_manifest.json](https://docs.microsoft.com/azure-sphere/app-development/app-manifest) file. The sample is set up to connect to the website `httpstat.us`:

    ```json
    "Capabilities": {
        "AllowedConnections": [ "httpstat.us" ],
      },
    ```

    You can revise the sample to connect to a different website for downloading, as described in the [Rebuild the sample to download from a different website](#rebuild-the-sample-to-download-from-a-different-website) section of this README.

1. Configure networking on your device. You must either [set up WiFi](https://docs.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

### Configure a static IP address

You can configure a static IP address on an Ethernet or a Wi-Fi interface. If you have configured a device with a static IP and require name resolution, your application must set a static DNS address. For more information, see the sections *Static IP address* and *Static DNS address* in [Use network services](https://docs.microsoft.com/azure-sphere/network/use-network-services).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

To start the download, press button A on the board. The sample downloads status information for HTTP statuses 200 (success) and 400 (bad request) from the `httpstat.us` website.

## Rebuild the sample to download from a different website

To download data from a website other than the default website, you'll need to get the root CA certificate from the website, modify the sample to use the new website and its certificate, and build and run the modified sample.

Complete the steps described in the following sections.

### Download the root CA certificate

If the website uses SSL, you may need to use a different root CA certificate. To download the certificate from the website, complete the following steps:

1. Open the browser and click the **Secure** icon, which is a padlock in the address bar.
1. If you're using Microsoft Edge, select **Connection is secure**; then click the certificate icon (highlighted yellow):

    ![certificate icon on web page](./media/download-certificate.png)

1. Select **Certificate**.
1. Open the **Certification Path** tab.
1. Select the top certificate in the hierarchy and then select **View Certificate**.
1. Open the **Details** tab and select **Copy to File**.
1. In the Certificate Export Wizard, click **Next**.
1. Select the Base-64 encoded X.509 (.CER) format and then click **Next**.
1. Type the file name to which to export the certificate and then click **Next**.
1. Click **Finish** to complete the wizard.
1. Rename the downloaded certificate file to have the .pem extension.

### Modify the sample to use the new website

Complete the following steps to modify the sample to use the new website.

1. In the `app_manifest.json` file, add the hostname of the new website to the **AllowedConnections** capability. For example, the following adds `Contoso.com` to the list of allowed websites.

    ```json
    "Capabilities": {
        "AllowedConnections": [ "httpstat.us", "Contoso.com"],
      },
    ```

1. Open `web_client.c`, go to the following statement, and change `https://httpstat.us/200?sleep=5000` and `https://httpstat.us/400?sleep=1000` to the URLs of the website you want to connect to.

    ```c
    // The web transfers executed with cURL.
    WebTransfer webTransfers[] = {
                // Download a web page with a delay of 5 seconds with status 200.
                {.url = "https://httpstat.us/200?sleep=5000", .easyHandle = NULL},
                // Download a web page with a delay of 1 second with status 400.
                {.url = "https://httpstat.us/400?sleep=1000", .easyHandle = NULL}};
    ```

1. Update the sample to use a different root CA certificate, if necessary:

     1. Put the trusted root CA certificate in the `certs/` folder (and optionally remove the existing `bundle.pem` certificate).
     1. Update line 16 of `CMakeLists.txt` to include the new trusted root CA certificate in the image package, instead of the `bundle.pem` certificate.
     1. Update line 195 of `web_client.c` to point to the new trusted root CA certificate.

### Build and run the sample modified to use the new website

To build and run the modified sample, follow the instructions in the [Build and run the sample](#build-and-run-the-sample) section of this README.

## Troubleshooting

The following message in device output may indicate an out of memory issue:  

`Child terminated with signal = 0x9 (SIGKILL)`

Currently, the Azure Sphere OS has a bug that causes a slow memory leak when using cURL and HTTPS. This slow memory leak can result in your application running out of memory. We plan to fix this bug in an upcoming quality release, and will announce it in the [IoT blog](https://techcommunity.microsoft.com/t5/internet-of-things/bg-p/IoTBlog) when it is available.

Until the updated OS is released, you can mitigate this problem. However, the mitigation might degrade performance, so you should remove it as soon as the updated OS is available. To mitigate the problem, disable the CURLOPT_SSL_SESSIONID_CACHE option when you create cURL handles, as shown in the following example: 

`curl_easy_setopt(curlHandle, CURLOPT_SSL_SESSIONID_CACHE, 0);`

For details about how to set this option, see [CURLOPT_SSL_SESSIONID_CACHE explained](https://curl.haxx.se/libcurl/c/CURLOPT_SSL_SESSIONID_CACHE.html) in the cURL documentation.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- For network troubleshooting, see [Troubleshoot network problems](https://docs.microsoft.com/azure-sphere/network/troubleshoot-network-problems).
