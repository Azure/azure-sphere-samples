# Sample: Wi-Fi (High-level app)

This sample application demonstrates how to connect to a Wi-Fi network and check the network status on an MT3620 device. After you configure the sample with your Wi-Fi network settings, you can use the buttons on the device to do the following:

- BUTTON_1 cycles through commands on the example Wi-Fi network in this order:

1. Add the network.
1. Disable the network.
1. Enable the network.
1. Delete the network.

- BUTTON_2 does the following:

1. Displays the network status of the device.
1. Lists the stored Wi-Fi networks on the device.
1. Starts a network scan.
1. Lists the available Wi-Fi networks.

The sample deduplicates the stored and scanned networks based on their SSID, security type and RSSID. Therefore, the output of the equivalent CLI commands `azsphere device wifi list` and `azsphere device wifi scan` might be different from the sample's output.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device. |
| [WifiConfig](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Manages Wi-Fi configuration on the device. |
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the IDE device output window during debugging.
| [EventLoop](https://docs.microsoft.com/en-gb/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

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

Access to a WPA2 (Wi-Fi Protected Access II) or an open Wi-Fi network is required.

The sample requires the following hardware:

- Azure Sphere device

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

## Prepare the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.01 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the WiFi_HighLevelApp sample in the WiFi folder.

## Set up hardware to display output

To prepare your hardware to display output from the sample, see "Set up hardware to display output" for [Windows](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) or [Linux](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output).

## Add your network settings

1. Open main.c.
1. Go to `static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";` and change `WIFI_NETWORK_SSID` to the SSID of the Wi-Fi network.
1. Go to `static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;` and change `WifiConfig_Security_Unknown` to the security type of the Wi-Fi network. If the network is open, set the security type to `WifiConfig_Security_Open`; otherwise, set it to `WifiConfig_Security_Wpa2_Psk`.
1. If the network is not an open network, go to `static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";` and change `WIFI_NETWORK_PASSWORD` to the password of your Wi-Fi network.

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build and deploy this sample:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

## Test the sample

The output will be displayed in the terminal window.

Use BUTTON_1 and BUTTON_2 as directed in the sample description, above.
