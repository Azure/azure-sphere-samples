---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Custom NTP
urlFragment: CustomNTP
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
- path: ethernet-setup-instructions.md
  target: ethernet-setup-instructions.md
description: "Demonstrates how to configure custom NTP servers on an MT3620 device."
---

# Sample: Custom NTP high-level app

This sample application demonstrates how to configure custom NTP servers on an MT3620 device.

The sample configures the NTP servers according to your configuration in the application manifest file. The last-time-synced information is retrieved when button A is pressed. The color of the LED indicates the time-synced status.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview)     | Manages button A (SAMPLE_BUTTON_1) and LED 2 on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     | Displays messages in the Device Output window during debugging. |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |

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
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.04 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

    `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *CustomNTP_HighLevelApp* sample in the *CustomNTP* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/customntp/).

1. Configure the sample application to work with your NTP server configuration. There are three different types of NTP server configurations possible:

    - System default (`prod.time.sphere.azure.net`) NTP server &mdash; See the [System default NTP server configuration](#system-default-ntp-server-configuration) section for setup instructions.
    - DHCP-assigned (Automatic) NTP server &mdash; See the [DHCP-assigned NTP server configuration](#dhcp-assigned-ntp-server-configuration) section for setup instructions.
    - Custom NTP server &mdash; See the [Custom NTP server configuration](#custom-ntp-server-configuration) section for setup instructions.

   The sample can be configured with any one type at a time. For details, see the section [Specifying an NTP Server](https://docs.microsoft.com/azure-sphere/app-development/rtc#specifying-an-ntp-server) in the topic [Manage system time and the RTC in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/rtc).

### Use Ethernet instead of Wi-Fi

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, follow the [Ethernet setup instructions](../../../ethernet-setup-instructions.md).

### System default NTP server configuration

In this configuration, the sample connects to the system default (`prod.time.sphere.azure.net`) NTP server.

To configure the sample to connect to the system default NTP server, add `"--TimeSource", "Default"` in the **CmdArgs** field of the `app_manifest.json` file.

The **CmdArgs** field should now look like the following: `"CmdArgs": [ "--TimeSource", "Default" ]`

### DHCP-assigned NTP server configuration

In this configuration, the sample connects to an NTP server that is assigned by DHCP.

To configure the sample to connect to a DHCP-assigned NTP server, make the following revisions in the `app_manifest.json` file:

1. Add `"--TimeSource", "Automatic"` in the **CmdArgs** field.
1. Fallback is enabled by default. To disable fallback, add  `"--DisableFallback"` in the  **CmdArgs** field. Configure this option only if you want fallback to be disabled.

The **CmdArgs** field should now look like the following:

- With fallback enabled: `"CmdArgs": [ "--TimeSource", "Automatic" ]`
- With fallback disabled: `"CmdArgs": [ "--TimeSource", "Automatic", "--DisableFallback" ]`

### Custom NTP server configuration

In this configuration, the sample connects to up to two user-configured NTP servers. If you do not have access to an NTP server for testing, see [NTP Pool Project](https://www.pool.ntp.org/) for a list of NTP servers you can use with the sample.

To configure the sample to connect to a primary NTP server and, optionally, a secondary NTP server, make the following revisions in the `app_manifest.json` file:

1. Add `"--TimeSource", "Custom"` in the **CmdArgs** field.
1. Fallback is enabled by default. To disable fallback, add  `"--DisableFallback"` in the  **CmdArgs** field. Configure this option only if you want fallback to be disabled.
1. Add `"--PrimaryNTPServer", "<hostname_or_ip>"` in the **CmdArgs** field and replace *`<hostname_or_ip>`* with the hostname or IP address of your primary NTP server.
1. If you want the sample to connect to a secondary NTP server, add `"--SecondaryNTPServer", "<hostname_or_ip>"` in the  **CmdArgs** field and replace *`<hostname_or_ip>`* with the hostname or IP address of your secondary NTP server.

The **CmdArgs** field should now look the following:

- With only a primary NTP server configured:

   - With fallback enabled: `"CmdArgs": [ "--TimeSource", "Custom", "--PrimaryNtpServer", "<hostname_or_ip>" ]`
   - With fallback disabled: `"CmdArgs": [ "--TimeSource", "Custom", "--DisableFallback", "--PrimaryNtpServer", "<hostname_or_ip>" ]`

- With a secondary NTP server configured:

   - With fallback enabled: `"CmdArgs": [ "--TimeSource", "Custom", "--PrimaryNtpServer", "<hostname_or_ip>", "--SecondaryNtpServer", "<hostname_or_ip>" ]`
   - With fallback disabled: `"CmdArgs": [ "--TimeSource", "Custom", "--DisableFallback", "--PrimaryNtpServer", "<hostname_or_ip>", "--SecondaryNtpServer", "<hostname_or_ip>" ]`

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../..//BUILD_INSTRUCTIONS.md).

### Test the sample

When the sample runs, it configures the NTP servers according to your configuration in the application manifest file. The output will be displayed in the terminal window. Use button A (SAMPLE_BUTTON_1) and the status LED as described below.

SAMPLE_BUTTON_1 does the following:

- Gets the last-time-synced information.
- If the device has not yet successfully time-synced, the sample logs a debug message informing the user that the device has not time-synced yet.
- If the device has successfully time-synced, the sample logs a message displaying the time before the sync, and the new adjusted time after the successful sync.

The color of the status LED indicates the following:

- Red &mdash; Not time-synced. This state is true on reboot till the device successfully time-syncs with the NTP server, or when the NTP server config is changed and the device has not yet synced with the NTP server.
- Green &mdash; The device has time-synced successfully to the NTP server. This state is true when the device successfully syncs with the configured NTP server.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- For network troubleshooting, see [Troubleshoot network problems](https://docs.microsoft.com/azure-sphere/network/troubleshoot-network-problems).
