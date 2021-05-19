# Sample: Avnet Azure Sphere Defult Project

This sample application was developed by Avnet to provide a fully functional Azure Sphere application that can easily be extended for your custom Azure Sphere application.  The application was built using the Microsoft AzureIoT example as a starting point.  The application includes some nice IoT Application features and implements methods to quickly add custom Device Twins, Direct Methods and Real Time application support.

This implementation is not the most efficient use of compute cycles or code space, but it provides a way to quickly develop a proof of concept Azure Sphere application.  Once developed, the application can be modified to realize additional efficiencies.

## Application Features

* Supports multiple Azure Connectivity options
   * Non-connected build for development activities (reads on-board sensors and outputs data to debug terminal)
   * IoT Hub Connection using the Azure Device Provisioning Service
   * IoT Edge support
   * Azure Plug and Play (PnP) support
   * Avnet IoTConnect Platform Support

### Sensors

* Sensors
   * The application generates and sends random data as telemetry to demonstrate how to send telemetry
   * ALS-PT19: Ambient light sensor (when using the M4 build option and loading the AvnetAlsPt19RTApp real time application)
   * Add additional sensors by adding real time applications or adding support directly into this high level application

### Button Features

The Avnet Starter Kit includes two user buttons, ButtonA and ButtonB

* When using the optional OLED display
   * Button A: Move to the last OLED screen
   * Button B: Move to the next OLED screen

### Connectivity Status

The Avnet Starter Kit RGB LED can be configured to show the IoTHub Connection status.  See the common/build_options.h file for details.

### Cloud Connectivity Options and Instructions

**IMPORTANT**: For all **connected** build options this application requires customization before it will compile and run. Follow the instructions linked below.

* [READMEAddDPS.md](READMEAddDPS.md)
* [READMEAddIoTEdge.md](READMEAddIoTEdge.md)
* [READMEStartWithIoTCentral.md](READMEStartWithIoTCentral.md)
* [READMEStartWithIoTHub.md](READMEStartWithIoTHub.md)

### Connected Features

* Sends sensor readings up as telemetry
* Implements Device Twins
   * Configure custom message to display on the optional OLED display
   * Configure real time applications to automatically send telemetry at the specified interval
   * Capture high level application memory high water mark since last reset
* Implements three direct methods
   * setTelemetryTxInterval: Modifies the period (in seconds) between the high level application sending telemetry messages
   * rebootDevice: Forces the device to execute a rebbot after a passed in delay (Seconds)
   * test: Demonstrates how to use the init and cleanup features of the Direct Method implementation
   
### Optional Hardware

The application supports the following optional hardware to enhance the example
* [0.96" I2C OLED LEC Display](https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
   * Verify that the pinout for your display matches the pinout on the Starter Kit

## Build Options

The application can be configured for multiple different deployments.  Build options are defined in the common/build_options.h header file.  To enable an option remove the comment delimiters ```//``` before the ```#define``` statement. 

### IOT_HUB_APPLICATION
* Enble for IoTConnect, IoTCentral, IoTHub and IoTEdge connected functionality

### USE_PNP
* Enable to use the Azure IoTHub Plug and Play functionality
* The project includes a PnP model in the Plug-N-Play folder.  To exercise the PnP interface using the Azure IoTExplorer tool, point Azure IoTExplorer to the Plug-N-Play folder.

### USE_IOT_CONNECT
* Enable to include the functionality required to connect to Avnet's IoTConnect platform

### OLED_1306
* Enable to include the functionality required to drive the optional OLED display

### M4_INTERCORE_COMMS
* Enable to include the functionality required to communicate with the partner M4 Realtime application that reads the on-board light sensor
* Read the details in m4_support.c

## WiFi or Ethernet

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, follow the instructions [here](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).

Add an Ethernet adapter to your hardware. A [ETH Click](https://www.mikroe.com/eth-click) board should be inserted into Click Socket #1.

## Prerequisites

The sample requires the following software:

- Azure Sphere SDK version 21.04 or higher. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- An Azure subscription. If your organization does not already have one, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

## Preparation

**Note:** By default, this sample targets the [Avnet Azue Sphere Starter Kit Rev1](http://avnet.me/mt3620-kit) board. To build the sample for the Rev2 board, change the Target Hardware Definition in the top level CMakeLists.txt file.

* Rev1: ```set(TARGET_HARDWARE "avnet_mt3620_sk_rev1")```
* Rev21: ```set(TARGET_HARDWARE "avnet_mt3620_sk_rev1")```

1. Set up your Azure Sphere device and development environment as described in [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
2. Clone the Azure Sphere Samples repository on GitHub and navigate to the Samples/AvnetDefaultProject/HighLevelExampleApp folder.
3. Connect your Azure Sphere device to your computer by USB.
4. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet: `azsphere device wifi add -s <your ssid> -p <your ssid password>  --targeted-scan`
5. Open an Azure Sphere Developer Command Prompt and enable application development on your device if you have not already done so: `azsphere device enable-development`
