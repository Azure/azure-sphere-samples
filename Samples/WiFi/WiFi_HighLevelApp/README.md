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

This sample application demonstrates how to connect to a Wi-Fi network and check the network status on an MT3620 device.

Button A (BUTTON_1) is used by the sample to perform Wi-Fi network-management operations, such as adding and enabling a Wi-Fi network. Button B (BUTTON_2) displays network-related information such as the device's network status. Each button press executes a particular operation on the device.

The sample displays the stored and scanned networks based on their SSID, security type, and RSSID, and removes duplicates. Therefore, the output of the equivalent CLI commands **azsphere device wifi list** and **azsphere device wifi scan** might be different from the sample's output.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the IDE device output window during debugging. |
| [networking](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |
| [wificonfig](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Manages Wi-Fi configuration on the device. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakePresets.json`   | CMake presets file, which contains the information to configure the CMake project. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for deploying and debugging the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

This sample requires the following items:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- Access to a WPA2 (Wi-Fi Protected Access II), an open Wi-Fi, or an EAP-TLS network.

- If your network is an EAP-TLS network, the Root CA certificate, the client certificate, and the private key must be installed on the device before you set up the sample for EAP-TLS.

   If you don't have certificates, follow the steps in [How to generate certificates for testing](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/Certificates/Cert_HighLevelApp/get-certificates.md) to create the certificates. You can install them in either of the following ways:

   - Use the **azsphere device certificate add** command, as described in [Store the certificates using the CLI](https://learn.microsoft.com/azure-sphere/network/eap-tls-cert-acquisition#store-the-certificates-using-the-cli).

   - Build and  run the [Cert_HighLevelApp sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/Certificates/Cert_HighLevelApp) but exit before the BUTTON_1 press that deletes the certificates.

## Setup

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 22.11 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://learn.microsoft.com/azure-sphere/install/install-sdk) if needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *WiFi_HighLevelApp* sample in the *WiFi* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/wifi/).

### Add your network settings to the sample

The sample must be configured to use the settings for your network. To add your network settings, complete the following steps:

1. In `main.c`, find the following line of code and replace `WIFI_NETWORK_SSID` with the SSID of your Wi-Fi network:

    ```c
    static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";
    ```

1. In `main.c`, find the following line of code:

    ```c
    static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;
    ```

1. In the code specified in the previous step, replace `WifiConfig_Security_Unknown` with the security type of your Wi-Fi network, as specified in the following steps. For a WPA2-PSK network or an EAP-TLS network, complete additional steps after you set the security type.

    - If the network is an open Wi-Fi network, set the security type to `WifiConfig_Security_Open`.

    - If the network is a WPA2-PSK network:

       1. Set the security type to `WifiConfig_Security_Wpa2_Psk`.
       1. Find the following line of code and replace `WIFI_NETWORK_PASSWORD` with the password of your Wi-Fi network.

           ```c
           static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";
           ```

    - If the network is an EAP-TLS network:

       **Note:** A root CA certificate and a client certificate must be installed, as described in [Prerequisites](#prerequisites).

       1. Set the security type to `WifiConfig_Security_Wpa2_EAP_TLS`.
       1. Find the following line of code and replace `SmplRootCACertId` with the identifier of your root CA certificate:

           ```c
           const char *rootCACertStoreIdentifier = "SmplRootCACertId";
           ```

           You can use the [**azsphere device certificate list**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#certificate-list) command to see the certificate IDs for all installed certificates.

       1. Find the following line of code and replace `SmplClientCertId` with the identifier of your client certificate:

           ```c
           const char *clientCertStoreIdentifier = "SmplClientCertId";
           ```

       1. Find the following line of code and replace `SmplClientId` with your client identity:

           ```c
           const char *clientIdentity = "SmplClientId";
           ```

       **Caution:** Because certificate IDs are system-wide, an **azsphere** command or a function call that adds a new certificate can overwrite a certificate that was added by an earlier command or function call, potentially causing network connection failures. We strongly recommend that you develop clear certificate update procedures and choose certificate IDs carefully. See [Certificate IDs](https://learn.microsoft.com/azure-sphere/app-development/certstore#certificate-ids) for more information about how Azure Sphere uses certificate IDs.

1. If you set the security type of the network to `WifiConfig_Security_Wpa2_EAP_TLS`, you must add the **EnterpriseWifiConfig** capability in the `app_manifest.json` file, as shown in the following code:

    ```json
    "EnterpriseWifiConfig": true
    ```

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

The output will be displayed in the terminal window. Use the buttons on the device to cycle through several operations related to Wi-Fi networks. Each button press executes a particular operation on the device.

Button A (BUTTON_1) performs the following management operations in the order specified:

1. Adds the network.
1. Enables the network.
1. Disables the network.
1. Duplicates the network.
1. Deletes the network.

Button B (BUTTON_2) performs the following display and scan operations in the order specified:

1. Displays the network status of the device.
1. Displays the network diagnostic information.
1. Lists the stored Wi-Fi networks on the device.
1. Starts a network scan.
1. Lists the available Wi-Fi networks.

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
- For an overview of Wi-Fi network connectivity, see [Networking connectivity overview](https://learn.microsoft.com/azure-sphere/network/wifi-including-ble).
- To learn more about EAP-TLS for Wi-Fi networks, see [Use EAP-TLS](https://learn.microsoft.com/azure-sphere/network/eap-tls-overview).
