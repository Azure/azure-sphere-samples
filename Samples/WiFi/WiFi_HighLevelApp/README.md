---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Wi-Fi
urlFragment: WiFi
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
description: "Demonstrates how to connect to a Wi-Fi network and check the network status on an Azure Sphere device."
---

# Sample: Wi-Fi high-level app

This sample application demonstrates how to connect to a Wi-Fi network and check the network status on an MT3620 device. After you configure the sample with your Wi-Fi network settings, you can use the buttons on the device to do the following:

BUTTON_1 cycles through commands on the example Wi-Fi network in this order:

1. Adds the network.
1. Enables the network.
1. Disables the network.
1. Duplicates the network.
1. Deletes the network.

BUTTON_2 does the following:

1. Displays the network status of the device.
1. Displays the network diagnostic information.
1. Lists the stored Wi-Fi networks on the device.
1. Starts a network scan.
1. Lists the available Wi-Fi networks.

The sample displays the stored and scanned networks based on their SSID, security type, and RSSID, and removes duplicates. Therefore, the output of the equivalent CLI commands `azsphere device wifi list`and `azsphere device wifi scan` might be different from the sample's output.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [gpio.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device. |
| [wificonfig.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Manages Wi-Fi configuration on the device. |
| [networking.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |
| [log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the IDE device output window during debugging.
| [eventloop.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events |

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

Access to a WPA2 (Wi-Fi Protected Access II), an open Wi-Fi, or an EAP-TLS network is required.

If the configured network is an EAP-TLS network, make sure that the Root CA certificate, the client certificate, and the private key are already 
installed on the device before you run the EAP-TLS portion of this sample. 

If you don't have certificates, follow the steps in [How to generate certificates for testing](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Certificates/Cert_HighLevelApp/get-certificates.md) to create the certificates. You can install them in either of the following ways:

- Use the **azsphere device certificate add** command, as described in [Store the certificates using the CLI](https://docs.microsoft.com/azure-sphere/network/eap-tls-cert-acquisition#store-the-certificates-using-the-cli).
- Build and  run the [Cert_HighLevelApp sample](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Certificates/Cert_HighLevelApp) but exit before the BUTTON_1 press that deletes the certificates.

The sample requires the following hardware:

- Azure Sphere device

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Prepare the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) if needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the WiFi_HighLevelApp sample in the WiFi folder.

## Add your network settings

Before you can run the sample, you need to configure the app to use the settings for your network. To add your network settings to the application, make the following changes to [main.c](./main.c):

1. Search for the line `static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";` and change `WIFI_NETWORK_SSID` to SSID of the Wi-Fi network.
1. Search for the line `static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;` and change `WifiConfig_Security_Unknown` to the security type of the Wi-Fi network. 
     - If the network is open:
        1. Set the security type to `WifiConfig_Security_Open`.
     - If the network is a WPA2-PSK network:
        1. Set the security type to `WifiConfig_Security_Wpa2_Psk` 
        1. Search for the line `static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";` and change `WIFI_NETWORK_PASSWORD` to the password of your Wi-Fi network.
     - If the network is an EAP-TLS network, you'll need an installed root CA certificate and a client certificate, as described in [prerequisites](#prerequisites).

        1. Set the security type to `WifiConfig_Security_Wpa2_EAP_TLS`
        1. Search for the line `const char *rootCACertStoreIdentifier = "SmplRootCACertId";` and change `SmplRootCACertId` to the identifier of your root CA certificate. You can use the [**azsphere device certificate list**][https://docs.microsoft.com/azure-sphere/reference/azsphere-device#certificate-list] command to see the certificate IDs for all installed certificates. 
        1. Search for the line `const char *clientCertStoreIdentifier = "SmplClientCertId";` and change `SmplClientCertId` to the identifier of your client certificate. 
        1. Search for the line `const char *clientIdentity = "SmplClientId";` and change `SmplClientId` to your client identity.

   **Caution:** Because certificate IDs are system-wide, an **azsphere** command or a function call that adds a new certificate can overwrite a certificate that was added by an earlier command or function call, potentially causing network connection failures. We strongly recommend that you develop clear certificate update procedures and choose certificate IDs carefully. See [Certificate IDs](https://docs.microsoft.com/azure-sphere/app-development/certstore#certificate-ids) for more information about how Azure Sphere uses certificate IDs. 

1. Open app_manifest.json.
1. If the security type of the configured network is `WifiConfig_Security_Wpa2_EAP_TLS`:
    1. Add the EnterpriseWifiConfig capability in the app_manifest.json file.

         `"EnterpriseWifiConfig": true`

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Test the sample

The output will be displayed in the terminal window.

Use BUTTON_1 and BUTTON_2 as directed in the sample description, above.
