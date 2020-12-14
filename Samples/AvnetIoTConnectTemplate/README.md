# Sample: Avnet IoTConnect Template

This sample demonstrates how to connect an Azure Sphere device to Avnet's IoTConnect platform.  The IoTConnect platform typically provides a selection of IoTConnect SDK's to use when connecting devices to the platform.  Because of the way Azure Sphere devices are architected, an IoTConnect SDK is not practical.  For that reason, we've created this example that can be used as a starting point, or reference when connecting Azure Sphere devices to the IoTConnect platform.

The example is a modified version of Microsoft's AzureIoT example; all the AzureIoT example features still work in this example.  The changes are detailed below . . . 

* Added supporting files
   * iotConnect.c
   * iotConnect.h
   * exit_codes.h 
* Add IoTConnectionInit() call from the InitPeripeheralsAndHandlers() routine
   * This routine sets up a periodic timer to send the IoTConnect Hello message
   * Attempts to read the last known sid from flash memory
* Calls IoTConnectConnectedToIoTHub()
   * Setup the callback routine for the cloud to device message handler
   * This routine receives IoTConnect specific details that are used when sending telemetry messages
   * Starts the periodic timer to kick off the IoTConnect Hello message transmission
* Modifies the SendTelemetry() routine to generate the IoTConnect specific telemetry message using the passed in telemetry JSON message

**Note**
The IoTConnect platform implements the IoTHub and DPS.  Users just need to upload and validate the Azure Sphere Tenant CA certificate for the platform to accept connections from your Azure Sphere Devices.  

More details will be avaliable as these features are deployed on the IoTConnect platform.

**IMPORTANT**: This sample application requires customization before it will compile and run. Follow the instructions in this README and in IoTCentral.md and/or IoTHub.md and/or IoTEdge.md to perform the necessary steps.

This application does the following:

- Sends simulated temperature telemetry data to Azure IoT Central or an Azure IoT hub at regular intervals.
- Sends a button-press event to Azure IoT Central or an Azure IoT hub when you press button A on the MT3620 development board.
- Sends a simulated orientation state to Azure IoT Central or an Azure IoT hub when you press button B on the MT3620 development board.
- Controls one of the LEDs on the MT3620 development board when you change a toggle setting on Azure IoT Central or when you edit the device twin on Azure IoT hub.

Before you can run the sample, you must configure either an Azure IoT Central application or an Azure IoT Hub, and modify the sample's application manifest to enable it to connect to the Azure IoT resources that you configured.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`

1. In main.c, ensure that the global constant networkInterface is set to "eth0". In source file AzureIoT/main.c, search for the following line:

   `static const char networkInterface[] = "wlan0";`

   and change it to:

   `static const char networkInterface[] = "eth0";`

1. In main.c, add the following lines before any other networking calls:

    ```c
     int err = Networking_SetInterfaceState("eth0", true);
     if (err == -1) {
           Log_Debug("Error setting interface state %d\n",errno);
           return -1;
       }
    ```

The sample uses the following Azure Sphere application libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Device Output window during debugging  |
| networking | Determines whether the device is connected to the internet |
| gpio | Manages buttons A and B and LED 4 on the device |
| storage | Opens the certificate file that is used to authenticate to the IoT Edge device |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Prerequisites

The sample requires the following software:

- Azure Sphere SDK version 20.10 or higher. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- An Azure subscription. If your organization does not already have one, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

## Preparation

**Note:** By default, this sample targets [Avnet Starter Kit Rev1](http://avnet.me/mt3620-kit) hardware. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../HardwareDefinitions/README.md).

1. Set up your Azure Sphere device and development environment as described in [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
1. Clone the Azure Sphere Samples repository on GitHub and navigate to the AzureIoT folder.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet.
1. Open an Azure Sphere Developer Command Prompt and enable application development on your device if you have not already done so:

   **azsphere device enable-development**

## Run the sample

- [Run the sample with Azure IoT Central](./IoTCentral.md)
- [Run the sample with an Azure IoT Hub](./IoTHub.md)
- [Run the sample with an IoT Edge device](./IoTEdge.md)
