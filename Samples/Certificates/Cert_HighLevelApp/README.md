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
description: "Shows how to use certificates in an Azure Sphere high-level application."
---

# Sample: Certificates high-level app

This sample shows how to use certificates in an Azure Sphere high-level application.

BUTTON_1 cycles through commands on the example certificates in this order:

1. Installs a client and a root certificate.
1. Installs a second root certificate.
1. Replaces the first root certificate with the second root certificate.
1. Reloads the Wi-Fi network (this step is required for an EAP-TLS network).
1. Deletes the certificates.

BUTTON_2 does the following:

1. Displays the available space for certificate storage on the device.
1. Displays the number of available certificates on the device.
1. Lists the certificate identifier, subject name, issuer name, and validity dates for each certificate that is installed on the device.

| Library | Purpose |
|---------|---------|
| [gpio.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) |Contains functions and types that interact with GPIOs.  |
| [log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |
| [certstore.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-certstore/certstore-overview) | Contains functions and types that interact with certificates. |
| [wificonfig.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Contains functions and types that interact with networking. |

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

This sample requires the following certificates in either PKCS1 or PKCS8 encoding and PEM format:

- Two Root CA certificates
- A client certificate, a client private key, and the private key password if one exists 

If you don't already have certificates for testing, follow the instructions in [How to generate certificates for use with samples](get-certificates.md) to create the certificates you'll need.

Refer to the following topics for information about how to manage certificates and configure an EAP-TLS network in a high-level Azure Sphere application:

- [Manage certificates in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/certstore)
- [Set up EAP-TLS network in an app](https://docs.microsoft.com/azure-sphere/network/eap-tls-app-setup)

The sample requires the following hardware:

- Azure Sphere device

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the Cert_HighLevelApp sample in the Certificates folder.

## Add your example certificates to the application

   **Caution!** Because [Certificate IDs](https://docs.microsoft.com/azure-sphere/app-development/certstore#certificate-ids) are system-wide, an **azsphere** command or a function call that adds a new certificate can overwrite a certificate that was added by an earlier 
   command or function call, potentially causing network connection failures. We strongly recommend that you develop clear certificate update procedures and choose 
   certificate IDs carefully.

To add your certificates to the application, make the following changes to [main.c](./main.c):

1. Add your root CA certificate:
   1. Open your root CA certificate in a text editor and copy the entire content of the certificate, including the -----BEGIN CERTIFICATE----- and -----END CERTIFICATE----- tags.
   1. In main.c, search for the line `static const char *rootCACertContent = "root_ca_cert_content";` and replace `root_ca_cert_content` with the content of your root CA certificate.
1. Add your second root CA certificate:
   1. Open your second root CA certificate in a text editor and copy the content of the certificate, including the tags.
   1. In main.c, search for the line `static const char *newRootCACertContent = "new_root_ca_cert_content";` and replace `new_root_ca_cert_content` with the content of your second root CA certificate.
1. Add your client certificate:
   1. Open your client certificate in a text editor and copy the content of the certificate, including the tags.
   1. In main.c, search for the line `static const char *clientCertContent = "client_cert_content";` and replace `client_cert_content` with the content of your client certificate.
1. Add your client private key:
   1. Open your client private key in a text editor and the copy the content of the private key, including the tags. The text of the tags will vary depending on whether the key uses PKCS1 or PKCS8 encryption.
   
      In addition, if you used PKCS1 encryption or followed the instructions in [How to generate certificates for use with samples](get-certificates.md), you must add a newline character (`\n') at the end of the third line of content. This line starts with "DEK-Info:" and is followed by a hyphenated string that starts with "DES-", a comma, and a 16-character hexadecimal value. Insert '\n' after the hexadecimal value, with no intervening spaces. If the content contains a blank line at this position, delete it.  
   1. In main.c, search for the line `static const char *clientPrivateKeyContent = "client_private_key_content";` and replace `client_private_key_content` with your client private key.
1. Add your client key password:
   1. In main.c, search for the line `static const char *clientPrivateKeyPassword = "client_private_key_password";` and replace `client_private_key_password` with the client private key password.

      **Note:** If the client key was created without a password, replace `client_private_key_password` with `NULL`.

**Note:** If your certificate content spans more than one line, add a line continuation character (`\`) at the end of each line. If you added a newline character to the private key content, add the line continuation character after the newline.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Test the sample

The output will be displayed in the terminal window.

Use BUTTON_1 and BUTTON_2 as directed in the sample description, above.

