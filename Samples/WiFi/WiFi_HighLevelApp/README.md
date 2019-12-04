# Sample: Wi-Fi (High-level app)

This sample application demonstrates how to connect to a Wi-Fi network and check the network status on an MT3620 device. After you configure the sample with your Wi-Fi network settings, you can use the buttons on the device to do the following:

- BUTTON_1 cycles through commands that add, disable, enable, and delete the example Wi-Fi network.
- BUTTON_2 to displays the Wi-Fi network configuration, and lists the available Wi-Fi networks.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device. |
| [WifiConfig](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-wificonfig/wificonfig-overview) | Manages Wi-Fi configuration on the device. |
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages the network configuration of the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging. |

## Prerequisites

Access to a WPA2 (Wi-Fi Protected Access II) or an open Wi-Fi network is required.

The sample requires the following hardware:

- Azure Sphere device

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

## Prepare the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the WiFi_HighLevelApp sample in the WiFi folder.
1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**

## Add your network settings

1. Open main.c.
1. Go to `static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";` and change `WIFI_NETWORK_SSID` to the SSID of the Wi-Fi network.
1. Go to `static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;` and change `WifiConfig_Security_Unknown` to the security type of the Wi-Fi network. If the network is open, set the security type to `WifiConfig_Security_Open`; otherwise, set it to WifiConfig_Security_Wpa2_Psk.
1. If the network is not an open network, go to `static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";` and change `WIFI_NETWORK_PASSWORD` to the password of your Wi-Fi network.

## Build the sample

1. Go to the **Build** menu, and select **Build All**. Alternatively, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

## Run the sample

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging.
1. Use BUTTON_1 and BUTTON_2 as directed below.

BUTTON_1 cycles through commands on the example Wi-Fi network in this order:

1. Add the network.
1. Enable the network.
1. Disable the network.
1. Delete the network.

BUTTON_2 does the following:

1. Displays the network status of the device.
1. Lists the stored Wi-Fi networks on the device.
1. Starts a network scan.
1. Lists the available Wi-Fi networks.

## Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.