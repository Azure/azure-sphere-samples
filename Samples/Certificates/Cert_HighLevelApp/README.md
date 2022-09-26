---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Certificates
urlFragment: Certificates
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
description: "Shows how to use and manage certificates in an Azure Sphere high-level application."
---

# Sample: Certificates high-level app

This sample demonstrates how to use and manage certificates in an Azure Sphere high-level application.

Button A (BUTTON_1) is used by the sample to perform certificate-management operations, such as certificate installation and deletion. Button B (BUTTON_2) displays certificate-related information such as the amount of space that is available on the device for certificate storage. Each button press executes a particular operation on the device.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [certstore](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-certstore/certstore-overview) | Contains functions and types that interact with certificates. |
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) |Contains functions and types that interact with GPIOs. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |
| [wificonfig](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Contains functions and types that interact with networking. |

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

The sample requires the following items:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- The following certificates in either PKCS1 or PKCS8 encoding and PEM format:

   - Two Root CA certificates

   - A client certificate, a client private key, and the private key password if one exists

   If you don't already have certificates for testing, follow the instructions in [How to generate certificates for use with samples](get-certificates.md) to create the certificates you'll need.

## Setup

1. Ensure that your Azure Sphere device is connected to your computer and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.09 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *Cert_HighLevelApp* sample in the *Certificates* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/certificates/).

### Add your example certificates to the application

   **Caution!** Because [Certificate IDs](https://learn.microsoft.com/azure-sphere/app-development/certstore#certificate-ids) are system-wide, an **azsphere** command or a function call that adds a new certificate can overwrite a certificate that was added by an earlier
   command or function call, potentially causing network connection failures. We strongly recommend that you develop clear certificate update procedures and choose
   certificate IDs carefully.

To add your certificates to the application, make the following changes in the sample:

1. Add your root CA certificate:

   1. Open your root CA certificate in a text editor and copy the entire content of the certificate, including the `-----BEGIN CERTIFICATE-----` and `-----END CERTIFICATE-----` tags.
   1. In `main.c`, find the following line of code and replace `root_ca_cert_content` with the content of your root CA certificate:

       ```c
       static const char *rootCACertContent = "root_ca_cert_content";
       ```

1. Add your second root CA certificate:

   1. Open your second root CA certificate in a text editor and copy the content of the certificate, including the tags.
   1. In `main.c`, find the following line of code and replace `new_root_ca_cert_content` with the content of your second root CA certificate:

       ```c
       static const char *newRootCACertContent = "new_root_ca_cert_content";
       ```

1. Add your client certificate:

   1. Open your client certificate in a text editor and copy the content of the certificate, including the tags.
   1. In `main.c`, find the following line of code and replace `client_cert_content` with the content of your client certificate:

       ```c
       static const char *clientCertContent = "client_cert_content";
       ```

1. Add your client private key:

   1. Open your client private key in a text editor and the copy the content of the private key, including the tags. The text of the tags will vary depending on whether the key uses PKCS1 or PKCS8 encryption.

      In addition, if you used PKCS1 encryption or followed the instructions in [How to generate certificates for use with samples](get-certificates.md), you must add a newline character (`\n') at the end of the third line of content. This line starts with "DEK-Info:" and is followed by a hyphenated string that starts with "DES-", a comma, and a 16-character hexadecimal value. Insert '\n' after the hexadecimal value, with no intervening spaces. If the content contains a blank line at this position, delete it.
   1. In `main.c`, find the following line of code and replace `client_private_key_content` with your client private key.

       ```c
       static const char *clientPrivateKeyContent = "client_private_key_content";
       ```

1. Add your client key password: In `main.c`, find the following line of code and replace `client_private_key_password` with the client private key password:

    ```c
    static const char *clientPrivateKeyPassword = "client_private_key_password";
    ```

    **Note:** If the client key was created without a password, replace `client_private_key_password` with `NULL`.

**Note:** If your certificate content spans more than one line, add a line continuation character (`\`) at the end of each line. If you added a newline character to the private key content, add the line continuation character after the newline.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

The output will be displayed in the terminal window. Use the buttons on the device to cycle through several operations related to certificates. Each button press executes a particular operation on the device.

Button A (BUTTON_1) performs the following management operations in the order specified:

1. Installs a client and a root certificate.
1. Installs a second root certificate.
1. Replaces the first root certificate with the second root certificate.
1. Reloads the Wi-Fi network (this step is required for an EAP-TLS network).
1. Deletes the certificates.

Button B (BUTTON_2) performs the following display operations in the order specified:

1. Displays the space that is available for certificate storage on the device.
1. Displays the number of available certificates on the device.
1. Lists the certificate identifier, subject name, issuer name, and validity dates for each certificate that is installed on the device.

## Next steps

- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
- For details about how an application can set up an EAP-TLS network, see [Set up EAP-TLS network from an app](https://learn.microsoft.com/azure-sphere/network/eap-tls-app-setup).
- To learn more about certificate management, see [Manage certificates in high-level applications](https://learn.microsoft.com/azure-sphere/app-development/certstore).
