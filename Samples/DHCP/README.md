---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – DHCP client
urlFragment: DHCP
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
description: "Demonstrates how to renew/release the current IP address on an MT3620 device."
---

# Sample: DHCP client high-level app

This sample application demonstrates how to renew or release the current IP address that the network's DHCP server has assigned to the MT3620 device.

The sample configures the desired network interface to be targeted, either Wi-Fi or Ethernet, according to the configuration in the application's global variable.
Buttons A and B are used to respectively *Release* and *Renew* the current IP address, while the color of the status LED will always indicate the network interface's status from the OS.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview)     | Manages button A (SAMPLE_BUTTON_1), Manages button B (SAMPLE_BUTTON_2) and LED 2 tri-channel on the device. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     | Displays messages in the Device Output window during debugging. |
| [networking](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which contains the project information and is required for all builds. |
| `CMakePresets.json`   | CMake presets file, which contains the information to configure the CMake project. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for deploying and debugging the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

The sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../HardwareDefinitions/README.md) file.
- The development board must be updated to Azure Sphere OS version 23.05 or above.

## Setup

Complete the following steps to set up this sample:

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the Internet.

1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 23.05 or above. At the command prompt, run `azsphere show-version` to check. Install the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.

1. Enable application development, if you have not already done so, by entering the `azsphere device enable-development` command at the command prompt.

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *DHCP_HighLevelApp* sample in the *DHCP* folder or download the zip file from the [Microsoft samples browser](https://learn.microsoft.com/samples/azure/azure-sphere-samples/dhcp/).

1. Configure the sample application to work with the desired network interface. There are two different types of network interface configurations possible (see the [network interface configuration](#network-interface-configuration) section for setup instructions):

    - System default, MT3620's on-chip Wi-Fi (`"wlan0"`).
    - External ethernet interface chip (`"eth0"`).

   The sample can be configured with any one type at a time, before compiling and running it.

1. Configure networking on your device. You must either [set up WiFi](https://learn.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://learn.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

### Network interface configuration

The default network interface is set to Wi-Fi, in the application's global variable `currentNetInterface`:

```c
/// <summary>
///     Available network interface device names.
/// </summary>
#define NET_INTERFACE_WLAN     "wlan0"
#define NET_INTERFACE_ETHERNET "eth0"

// User configuration.
const char *const currentNetInterface = NET_INTERFACE_WLAN;
```

To switch to the Ethernet interface, simply assign the `NET_INTERFACE_ETHERNET` value to the `currentNetInterface` global variable, which can be found in `main.c`.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

### Test the sample

When the sample runs, it loops until it successfully enables the configured network interface, according to your configuration in the application's global variable `currentNetInterface`, while disabling the other one available.

The output will be displayed in the terminal window, and it will show any transition in the current network interface's status, according to the statuses enumerated in the OS's [Networking_InterfaceConnectionStatus](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/enum-networking-interfaceconnectionstatus) enum type.

Here is an example of the output, using both 'wlan0' and 'eth0' interfaces:

```cmd
Remote debugging from host 192.168.35.1, port 60337
INFO: DHCP High Level Application starting.
INFO: Successfully initiated peripherals.
INFO: Network interface 'wlan0' is connected to the Internet (local IP address [192.168.0.135]).
INFO: Successfully released the IP address.
INFO: Network interface 'wlan0' is connected to the network (no IP address assigned).
INFO: Successfully renewed the IP address.
INFO: Network interface 'wlan0' is connected and has been assigned IP address [192.168.0.135].
INFO: Network interface 'wlan0' is connected to the Internet (local IP address [192.168.0.135]).
```

```cmd
Remote debugging from host 192.168.35.1, port 57127
INFO: DHCP High Level Application starting.
INFO: Successfully initiated peripherals.
ERROR: Network interface 'eth0' NOT ready!
INFO: Attempting to enable network interface 'eth0'.
INFO: Network interface is now set to 'eth0'.
INFO: Network interface 'eth0' is up but not connected to the network.
INFO: Network interface 'eth0' is connected to the network (no IP address assigned).
INFO: Network interface 'eth0' is connected and has been assigned IP address [192.168.0.135].
INFO: Network interface 'eth0' is connected to the Internet (local IP address [192.168.0.242]).
INFO: Successfully released the IP address.
INFO: Network interface 'eth0' is connected to the network (no IP address assigned).
INFO: Successfully renewed the IP address.
INFO: Network interface 'eth0' is connected to the Internet (local IP address [192.168.0.242]).
```

**Note:** as noticeable above, depending on the network setup/speed, some network interface state transitions may not appear in the output if they occur between the timer's interleaved checks..

- Press **Button A** to **release** the current IP address assigned by the network's DHCP server.

  **Note**: by design, once the IP address is released, the network interface will not be automatically reassigned an IP address by the DHCP server, until the *renew IP address* is explicitly requested (i.e. by pressing **Button B**), or a device restart (or power-cycle) is performed.

- Press **Button B** to **renew** the current IP address assigned by the network's DHCP server, or request one if previously released.

The color of the status LEDs indicates the following:

|LED color|State|
|-|-|
| Turned off | The network interface is unavailable.
| Red | The network interface is up and available, but hasn't yet connected to the network.
| Yellow (Red + Green) | The network interface is up and connected to the network, but hasn't yet received an IP address from the network's DHCP server.
| Blue | The network interface is up, connected to the network and has successfully acquired an IP address from the network's DHCP server.
| Green | The network interface is fully operative and connected up to the Internet.
|

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).
- To connect an Azure Sphere device to Ethernet, see [Connect Azure Sphere to Ethernet](https://learn.microsoft.com/azure-sphere/network/connect-ethernet?tabs=cliv2beta).
- For specific networking APIs specifications, see [Applibs networking.h](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview).
- For network troubleshooting, see [Troubleshoot network problems](https://learn.microsoft.com/azure-sphere/network/troubleshoot-network-problems).
