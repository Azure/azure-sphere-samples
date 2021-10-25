---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Private network services
urlFragment: PrivateNetworkServices
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
description: "Demonstrates how to connect an Azure Sphere device to a private network and use network services."
---

# Sample: Private network services

This sample application demonstrates how you can [connect an Azure Sphere device to a private network](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) and [use network services](https://docs.microsoft.com/azure-sphere/network/use-network-services). It configures the Azure Sphere device to run a DHCP server and an SNTP server, and implements a basic TCP server.

The DHCP and SNTP servers are managed by the Azure Sphere OS and configured by the high-level application. The servers start only upon request from the application but continue to run even after the application stops.

The TCP server runs in the application process and stops when the application stops. Note that this sample TCP server implementation is basic, for illustration only, and that it does not authenticate or encrypt connections; you should replace it with your own production logic.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) |  Displays messages in the Device Output window during debugging. |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Gets and sets network interface configuration. |

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

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../HardwareDefinitions/README.md) file.

## Setup

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 21.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *PrivateNetworkServices* sample in the *PrivateNetworkServices* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/privatenetworkservices/).
1. Configure the sample for your network interface. This sample will run on any supported network interface. However, it is configured by default for a private Ethernet network.

    - To use Ethernet, you must connect and configure an Ethernet adapter to your MT3620 development board. See [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) for instructions.

    - To use a different network interface, change the value of the global constant **NetworkInterface** in the source file `PrivateNetworkServices\main.c`. For example, to specify a Wi-Fi network, replace `eth0` with `wlan0` in the following code in `main.c`:

       ```c
       static const char NetworkInterface[] = "eth0";
       ```

1. If you want the sample to set and retreive the sizes of the socket send/receive buffers, add code that uses the standard **getsockopt** and **setsockopt** functions.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

### Test the sample

This sample configures the Azure Sphere device to run a DHCP server and an SNTP server, and implements a basic TCP server.

Complete the following steps to test this functionality:

1. Connect your computer to the same private network to which you connected your device. If using an Ethernet private network, attach an Ethernet cable from the Ethernet adapter on the device to the Ethernet connection on your computer.
1. If your computer is managed by policies that prevent it from being connected to multiple network interfaces at once, you may need to disable other network interfaces while using this sample.
1. Perform the following operations, which are described in the sections below:

    - [Test the device's DHCP server](#test-the-devices-dhcp-server).
    - [Test the device's SNTP server](#test-the-devices-sntp-server).
    - [Test the application's TCP server](#test-the-applications-tcp-server).

**Note:** The sample uses the IP address range 192.168.100.xxx. If you have another network adapter that uses the same range, you will need to either modify the sample or disable the other network adapter temporarily.

#### Test the device's DHCP server

To test the device's DHCP server, verify that the server has issued an IP address to your computer for its network connection.

1. Open a command prompt on your computer.
1. Enter a command that displays the IP addresses for the adapters. For example, enter `ipconfig` if you're using Windows.

   You should see that the DHCP server has issued an IP address in the 192.168.100.xxx range to your computer, as shown in the following sample output of `ipconfig`:

   ```
   <network interface type> adapter <name>:

      Connection-specific DNS Suffix  . :
      Link-local IPv6 Address . . . . . : fe80::8c67:be24:4d9a:d4bb%9
      IPv4 Address. . . . . . . . . . . : 192.168.100.11
      Subnet Mask . . . . . . . . . . . : 255.255.255.0
      Default Gateway . . . . . . . . . :
   ```

1. If an IP address was not issued to your computer, enter a command that requests the DHCP server to issue a new IP address. For example, enter `ipconfig  /renew` if you're using Windows.

Alternatively, you can use a DHCP client test tool (not provided) on your computer to inspect the DHCP server response in more detail. For example, you may want to look at the NTP server addresses returned.

#### Test the device's SNTP server

To test the device's SNTP server, verify that the server responds to a query for the offset between your computer's time and the device's time.

1. Ensure the Azure Sphere device is connected to the internet via a different network interface (for example, Wi-Fi if using private Ethernet), so that it can obtain time settings from a public NTP server. The SNTP server won't respond until it knows the current time.
1. Open a command prompt on your computer.
1. Enter a command that displays the difference between your computer's time and the device's time. For example, enter the following command if you're using Windows:

   ```
   w32tm /stripchart /computer:192.168.100.10 /dataonly /samples:1
   ```

   Sample output of the `w32tm` command is as follows:

   ```
   Tracking 192.168.100.10 [192.168.100.10:123].
   Collecting 1 samples.
   The current time is 06/02/2019 14:18:09.
   14:18:09, +00.0349344s
   ```

   For details about the `w32tm` command, see [Windows Time tool w32tm](https://docs.microsoft.com/windows-server/networking/windows-time-service/windows-time-service-tools-and-settings#windows-time-service-tools).

1. If the SNTP server doesn't respond, check that the app is running and that the Azure Sphere device is connected to the internet.

   You may see an error similar to the one shown in the following `w32tm` command output when the SNTP server doesn't respond.

   ```
   Tracking 192.168.100.10 [192.168.100.10:123].
   Collecting 1 samples.
   The current time is 06/02/2019 14:16:50.
   14:16:50, error: 0x800705B4
   ```

#### Test the application's TCP server

To test the application's TCP server, verify that the characters you type are received and acknowledged by the server.

1. Ensure that the sample app is still running on your Azure Sphere device.
1. On your computer, use a terminal emulator to open a raw TCP connection to the application's TCP server at 192.168.100.10 port 11000. You can open this connection with a third-party terminal emulator, such as PuTTY (using a *raw* connection type), or with the Windows Telnet client.

   To use the Windows built-in Telnet client:

   1. Open Control Panel and select **Programs and Features** > **Turn Windows features on or off** to launch the **Windows Features** window.
   1. Ensure **Telnet Client** is selected and select **OK**.
   1. Open a command prompt and enter `telnet 192.168.100.10 11000`.

1. Type characters in the terminal. The application's TCP server can hold a maximum of 15 characters before it expects a newline character.

   If you're using Visual Studio, the characters that are received by the server will appear in the debug console—either immediately or when you press the *Enter* key, which puts a newline character in the buffer.

   If the server receives a 16th character before a newline character has been received, the previous 15 characters are discarded, the newly-arrived character is placed at the start of the buffer, and the output window in Visual Studio displays the following text:

   `Input data overflow. Discarding 15 characters.`

1. Watch the terminal for a response from the server when you press the *Enter* key. You should see the following response from the server:

   `Received "<last-received-line-of-text>"`

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- For network troubleshooting, see [Troubleshoot network problems](https://docs.microsoft.com/azure-sphere/network/troubleshoot-network-problems).
