# Sample: AvnetRSL10Sensor

This sample implements a application that can monitor and report sensor data for upto 10 RSL10 BLE sensors devices.

This sample uses a BLE device connected to an Azure Sphere device to capture, parse and transmit RSL10 Sensor data to Azure.  The Avnet BLE PMOD runs a modified Laird SmartBasic application (Repeater Gateway) that listens for broadcast messages from any RSL10 device, then transmits the message over a UART connection to the Azure Sphere Starter Kit.  The Azure Sphere application parses the message, verifies that the RSL10 is authorized to send data from the application and sends up any telemetry/events as telemetry to Azure.

The application will only send telemetry if it's recieved updated data from one of the RSL10 devices, and it will only send data for the device that it received data from.

The default telemetry send period is 2 seconds.  That is as long as new data is received within the 2-second window, telemetry will be sent every 2-seconds.

## Required hardware

* [Avnet Azure Sphere Starter Kit](http://avnet.me/mt3620-kit)
* [Avnet BLE PMOD](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-pmod-nrf-ble-g-3074457345642996769)
* [ON RSL10](https://www.avnet.com/shop/us/products/on-semiconductor/rsl10-sense-gevk-3074457345639458682/)

## Optional hardware

* [0.96" I2C OLED LEC Display](https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
   * Verify that the pinout for your display matches the pinout on the Starter Kit
   * See the Starter Kit Users Guide for details on attaching the OLED to your Starter Kit

## Hardware Configuration

### Update the BLE PMOD fw/sw

The BLE PMOD needs to be updated with the latest SmartBasic firmware and then loaded with the SmartBasic application.

* Update the PMOD's onboard BL654 module to FW Version 29.4.6.0 or newer
   * [Link to firmware, scroll way down to documentation section](https://www.lairdconnect.com/wireless-modules/bluetooth-modules/bluetooth-5-modules/bl654-series-bluetooth-module-nfc)
* Load the $autorun$.BT510.gateway.sb SmartBasic application onto the BL654 module
   * This custom SmartBasic application is located in this repo under the RSL10Repeater folder
   * Build and load the $autorun$.BT510.gateway.sb application
  
### Connect the BLE PMOD to the StarterKit

* Solder a 2x6 Right angle header onto the Starter Kit
* Plug the updated BLE PMOD into the new 2x6 header
* Update the CMakeLists.txt file with your target hardware platform
   * avnet_mt3620_sk OR
   * avnet_mt3620_sk_rev2 OR
   * qiio-200-dev
![Starter Kit](./media/ConnectorTop.jpg)

## Build Options

The application behavior can be defined in the build_options.h file

* TARGET_QIIO_200 enables support for using the cellular enabled qiio-200 Azure Sphere Development kit 
   * See the sample_appliance.h file in the hardware/qiio-200-dev folder for interface details
* USE_ETH0 enables support for an optional ETH Click board in Click Site 1
* USE_IOT_CONNECT enables support to connect to Avnet IoTConnect Cloud solution platform

## Runtime configuration

The application is configured from the cloud

### Authorize RSL10s to connect to the appliation

Since this device could be deployed in an enviornment with many different RSL10 devices, you must explicitly authorize/identify the devices that the application will use for telemetry reporting.  To authorize a RSL10 device enter the BLE MAC address into one of the avaliable device twins labeled "authorizedMacX" where X is a value between 1-10.  The MAC should be in upper case and in the following format:  ```C9:63:9C:48:6E:A6```

Note that the default mode of the application is to automatically allow any RSL10 device to connect.  This is so that you can see the MAC address for any device in the imediate area.  You can see the MAC by either monitororing the telemetry data in the cloud, or if you have an optional OLED display on your Starter Kit, it will also be displayed there by selecting the RSL10 screen using the A and B buttons.

### Avnet's IoTConnect configuration

If you're using Avnet's IoTConnect cloud solution you can use the device template JSON file located in the IoTConnect folder to define all the device to Cloud (D2C) messages and device twins.

### Configure the application to connect to your Azure Solution

Use the instructions below to configure your app_manifest.json file to allow a connection to an Azure IoT Hub.

**IMPORTANT**: This sample application requires customization before it will compile and run. Follow the instructions in this README and in IoTCentral.md and/or IoTHub.md to perform the necessary steps.

This application does the following:

Before you can run the sample, you must configure either an Azure IoT Central application or an Azure IoT hub, and modify the sample's application manifest to enable it to connect to the Azure IoT resources that you configured.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`

1. In main.c, ensure that the global constant NetworkInterface is set to "eth0". In source file AzureIoT/main.c, search for the following line:

   `static const char NetworkInterface[] = "wlan0";`

   and change it to:

   `static const char NetworkInterface[] = "eth0";`

1. In main.c, add the following lines before any other networking calls:

    ```c
     int err = Networking_SetInterfaceState("eth0", true);
     if (err == -1) {
           Log_Debug("Error setting interface state %d\n",errno);
           return -1;
       }
    ```

## Prerequisites

The sample requires the following software:

- Azure Sphere SDK version 21.01 or higher. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- An Azure subscription. If your organization does not already have one, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

## Preparation

**Note:** By default, this sample targets the [Avnet Azure Sphere Starter Kit](http://avnet.me/mt3620-kit) hardware.  To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../HardwareDefinitions/README.md).

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
2. Clone the Azure Sphere Samples repository on GitHub and navigate to the AvnetRSL10Sensor folder.
3. Connect your Azure Sphere device to your computer by USB.
4. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet.
5. Open an Azure Sphere Developer Command Prompt and enable application development on your device if you have not already done so:

   **azsphere device enable-development**

## Run the sample

- [Run the sample with Azure IoT Central](./IoTCentral.md)
- [Run the sample with an Azure IoT Hub](./IoTHub.md)
