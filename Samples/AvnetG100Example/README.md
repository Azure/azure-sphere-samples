# Sample: Avnet Azure Sphere Guardian 100 Default Project

This sample application was developed by Avnet to provide a fully functional Azure Sphere Guardian 100 application that can easily be extended for your custom Guardian 100 application.  The application was built using the Microsoft AzureIoT example as a starting point.  The application includes some nice IoT Application features and implements methods to quickly add custom Device Twins, Direct Methods and Real Time application support.

This implementation is not the most efficient use of compute cycles or code space, but it provides a way to quickly develop a proof of concept G100 Azure Sphere application.  Once developed, the application can be modified to realize additional efficiencies.

## Application Features

* Supports multiple Azure Connectivity options
   * Non-connected build for development activities (reads on-board sensors and outputs data to debug terminal)
   * IoT Hub Connection using the Azure Device Provisioning Service
   * IoT Edge support
   * Azure Plug and Play (PnP) support
   * Avnet IoTConnect Platform Support

### Cloud Connectivity Options and Instructions

**IMPORTANT**: For all **connected** build options this application requires customization before it will compile and run. Follow the instructions linked below.

* [READMEAddDPS.md](READMEAddDPS.md)
* [READMEAddIoTEdge.md](READMEAddIoTEdge.md)
* [READMEStartWithIoTCentral.md](READMEStartWithIoTCentral.md)
* [READMEStartWithIoTHub.md](READMEStartWithIoTHub.md)

### Sensors

* The G100 does not include any built in sensors.  There is logic to send simulated telemetry data to demonstrate where and how to send telemetry data.

### Connectivity Status

The application can be configured to use the G100 user LEDs to show the IoTHub Connection status.  See the common/build_options.h file for details.

### Connected Features

* Sends simulated sensor readings up as telemetry
* Implements read/write Device Twins
   * ```sensorPollPeriod```: Configure how often the read sensor routine runs.  This routine is currently empty.
   * ```telemetryPeriod```: Configure how often the telemetry data is sent. 
   * ```realTimeAutoTelemetryPeriod```: Configure real time applications to automatically send telemetry at the specified interval
   * ```enableUartDebug```: Enable/disable sending application debug out the G100 external USB/UART
* Implements read only Device Twins
   * ```versionString```
   * ```manufacturer```
   * ```model```
   * Wi-Fi ```ssid```
   * Wi-Fi ```freq```
   * Wi-Fi ```bssid```
   * ```MemoryHighWaterKB```: Capture high level application memory high water mark since last reset
* Implements three direct methods
   * ```setTelemetryTxInterval```: Modifies the period (in seconds) between the high level application sending telemetry messages
   * ```rebootDevice```: Forces the device to execute a reboot after a passed in delay (Seconds)
   * ```test```: Demonstrates how to use the init and cleanup features of the Direct Method implementation
   
### Code Base Features
#### Device Twins
Developers can quickly add new device twin items to the application by adding an entry into a table and either using pre-built handlers by data type or creating custom handlers to fit any need.

    // Define each device twin key that we plan to catch, process, and send reported property for.
    // .twinKey - The JSON Key piece of the key: value pair
    // .twinVar - The address of the application variable keep this key: value pair data
    // .twinFD - The associated File Descriptor for this item.  This is usually a GPIO FD.  NULL if NA.
    // .twinGPIO - The associted GPIO number for this item.  NO_GPIO_ASSOCIATED_WITH_TWIN if NA
    // .twinType - The data type for this item, TYPE_BOOL, TYPE_STRING, TYPE_INT, or TYPE_FLOAT
    // .active_high - true if GPIO item is active high, false if active low.  This is used to init the GPIO 
    // .twinHandler - The handler that will be called for this device twin.  The function must have the signaure 
    //                void <yourFunctionName>(void* thisTwinPtr, JSON_Object *desiredProperties);
    // 

    twin_t twinArray[] = {
        {
            .twinKey = "appLed",
            .twinVar = &appLedIsOn,
            .twinFd = &appLedFd,
            .twinGPIO = SAMPLE_APP_LED,
            .twinType = TYPE_BOOL,
            .active_high = false,
            .twinHandler = (genericGPIODTFunction)
        },
        {
            .twinKey = "sensorPollPeriod",
            .twinVar = &readSensorPeriod,
            .twinFd = NULL,
            .twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,
            .twinType = TYPE_INT,
            .active_high = true,
            .twinHandler = (setSensorPollTimerFunction)
        },   
        {
            .twinKey = "telemetryPeriod",
            .twinVar = &sendTelemetryPeriod,
            .twinFd = NULL,
            .twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,
            .twinType = TYPE_INT,
            .active_high = true,
            .twinHandler = (setTelemetryTimerFunction)
        },
    };
   
#### Direct Methods
Developers can quickly add new direct methods to the application by adding an entry into a table and implementing custom handlers for initialization, run time execution and exit/cleanup.  The implementation manages identifying and executing the direct method routine as well as either passing a custom response string or providing a canned response string to the IoTHub.

    // Define each direct method that we plan to process
    // .dmName - The direct method name
    // .dmPayloadRequired - Does the direct method require a payload?
    // .dmInit - Init function called at power up, NULL if not required
    // .dmHandler: The handler that will be called for this direct method.  The function must have the same signaure 
    //     void <yourFunctionName>(void* thisTwinPtr, JSON_Object *desiredProperties);
    // .dmCleanup - The handler that will be called at application exit time, NULL is not required 
    
    direct_method_t dmArray[] = {
        {
            .dmName = "test",
            .dmPayloadRequired=true,
            .dmInit=dmTestInitFunction,
            .dmHandler=dmTestHandlerFunction,
            .dmCleanup=dmTestCleanupFunction
        },
        {
            .dmName = "rebootDevice",
            .dmPayloadRequired=false,
            .dmInit=dmRebootInitFunction,
            .dmHandler=dmRebootHandlerFunction,
            .dmCleanup=dmRebootCleanupFunction
        },
        {
            .dmName = "setTelemetryTxInterval",
            .dmPayloadRequired=true,
            .dmInit=NULL,
            .dmHandler = dmSetTelemetryTxTimeHandlerFunction,
            .dmCleanup=NULL
        }
    };
   
#### Real time application integration
Developers can quickly add support for real time applications running on one of the MT3620 M4 cores.  Avnet has provided a small library of working real time applications that are supported as well as a template application that can be modified to use with this sample.

    // .m4Name (string): The name of the application, used for debug and to make the table more readable
    // .m4RtComponentID (GUID string): The component ID of the M4 application
    // .m4InitHandler (function name): The routine that will be called on startup for this real time application
    // .m4Handler (function name): The handler that will be when data is received from the M4 application
    // .m4RawDataHandler (function name) : The handler that knows how to process the M4 application's raw data structure
    // .m4TelemetryHandler (function name): The routine that will be called to request telemetry from the real time application
    // .m4Cleanup (function name): The routine that will be called when the A7 application exits
    // .m4InterfaceVersion (INTER_CORE_IMPLEMENTATION_VERSION): The implementation version (for future use)

    m4_support_t m4Array[] = {

        // The AvnetGenericRTApp demonstrates how to use this common interface
        {
            .m4Name="AvnetGenericRTApp",
            .m4RtComponentID="9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
            .m4InitHandler=genericM4Init,
            .m4Handler=genericM4Handler,
            .m4rawDataHandler=referenceRawDataHandler,
            .m4CleanupHandler=genericM4Cleanup,
            .m4TelemetryHandler=genericM4RequestTelemetry,
            .m4InterfaceVersion=V0
        },
    };
   
## Build Options

The application can be configured for multiple different deployments.  Build options are defined in the common/build_options.h header file.  To enable an option remove the comment delimiters ```//``` before the ```#define``` statement. 

### ```IOT_HUB_APPLICATION```
* Enble for IoTConnect, IoTCentral, IoTHub and IoTEdge connected functionality

### ```USE_PNP```
* Enable to use the Azure IoTHub Plug and Play functionality
* The project includes a PnP model in the Plug-N-Play folder.  To exercise the PnP interface using the Azure IoTExplorer tool, point Azure IoTExplorer to the Plug-N-Play folder.

### ```USE_IOT_CONNECT```
* Enable to include the functionality required to connect to Avnet's IoTConnect platform

### ```M4_INTERCORE_COMMS```
* Enable to include the functionality required to communicate with the partner M4 real time application that reads the on-board light sensor
* Read the details in m4_support.c

### ```ENABLE_DEBUG_TO_UART```
* Enable to include logic and sample calls to send application debug to the G100 external USB/UART.  This option also includes an additional device twin to control the feature from the cloud

### ```ENABLE_UART_RX```
* Enable to include logic to read character data from the G100 external USB/UART.  Data is read until a '\n' new line charcter is encountered.  If the message is valid JSON, then the message is sent to the IoTHub as telemetry.  If the data is not valid JSON, it's output to debug.

## Prerequisites

The sample requires the following software:

- Azure Sphere SDK version 21.04 or higher. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- An Azure subscription. If your organization does not already have one, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).
