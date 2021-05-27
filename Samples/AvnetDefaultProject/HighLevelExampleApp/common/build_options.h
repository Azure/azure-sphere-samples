/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Connectivity options
//
//  IOT_HUB_APPLICATION: Enable for any configuration that connects to an IoTHub/IoTCentral.
//  USE_IOT_CONNECT: Enable to connect to Avnet's IoTConnect Cloud solution.
//
//  USE_PNP: Enable to buid a PNP compatable application.  Note that the user must define, validate and 
//           publish the PnP model to Microsoft's GitHub repo
//           https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play                                    
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// If your application is going to connect straight to a IoT Hub or IoT Connect, then enable this define.
//#define IOT_HUB_APPLICATION

// Define to build for Avnet's IoT Connect platform
//#define USE_IOT_CONNECT

// If this is a IoT Conect build, make sure to enable the IOT Hub application code
#ifdef USE_IOT_CONNECT
#define IOT_HUB_APPLICATION
#define IOT_CONNECT_API_VERSION 1
#endif 

// Define if you want to build the Azure IoT Hub/IoTCentral Plug and Play application functionality
// #define USE_PNP

// Make sure we're using the IOT Hub code for the PNP configuration
#ifdef USE_PNP
#define IOT_HUB_APPLICATION

// Use this model for testing and point the Azure IoT Explorer to the project/PlugNPlay directory
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:avnet:defaultValidation;1" // https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play                                   

#else
// Define a NULL model ID if we're not building for PnP
#define IOT_PLUG_AND_PLAY_MODEL_ID ""

#endif 

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Optional Hardware options
//
//  OLED_SD1306: Enable to add OLED display functionality to the project
//
//  To use an optional OLED dispolay
//  Install a 128x64 OLED display onto the unpopulated J7 Display connector
//  https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
//
//////////////////////////////////////////////////////////////////////////////////////////////////

//#define OLED_SD1306

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Use the Avnet Starter Kit RGB LED to show network connection status
//
//  USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS: Enable to add logic to drive the Avnet Starter Kit
//  RGB LED to show network status.
//
//  Note: This feature is only avaliable when building IOT_HUB_APPLICATIONs 
//
//  Red: No wifi connection
//  Green: Wifi connection, not connected to Azure IoTHub
//  Blue: Wifi connected and authenticated to Azure IoTHub (Blue is good!)
//
//////////////////////////////////////////////////////////////////////////////////////////////////

//#define USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Track telemetry TX status and resend on reconnect
//
//  ENABLE_TELEMETRY_RESEND_LOGIC: Enable to add logic that will track telemetry send status and
//  will attempt to resend un-sent telemetry data when the application reconnects to the IoTHub
//
//  Note: This feature is only avaliable when building IOT_HUB_APPLICATIONs 
//
//  Feature Overview:
// 
//  Startup Logic: When the applicaion starts an empty linked list is created and a callback is
//  configured called AzureIoT_SendTelemetryCallback().  This callback will be called when a 
//  telemetry send message has been successfully transmitted to the IoTHub.  Note that this callback
/// does NOT get called when the telemetry send fails.
//
//  Runtime Logic:  When the application sends telemetry using the Cloud_SendTelemetry() a new node is
//  added to the telemetry linked list capturing the telemetry JSON string.  When the routine sends the
//  telemetry using the AzureIoT_SendTelemetry() function, a pointer to the linked list node is passed
//  in as a void* context pointer. 
//
//  If the telemetry is successfully sent to the IoTHub, then AzureIoT_SendTelemetryCallback() is 
//  instantiated and includes the void* context pointer that refers to the linked list node.  At this time
//  the node is deleted from the list, since the message was sent.
//
//  In the happy path, this linked list would always have one item in the list and only for a short period
//  of time.  That's the time between when the application sends the message and when the callback is called
//  informing the application that the message was sent.
//
//  In the un-happy path, the telemetry message is not sent for some reason.  Maybe the network connection
//  went down, or the connection to the IoTHub is disrupted.  In this case, any telemetry messages that the
//  application attempts to send will be captured in the linked list.  
//
//  When ConnectionChangedCallbackHandler() is instantiated, the routine checks to see if the telemetry list 
//  contains any nodes.  If so, then the logic will attempt to send the telemetry messages again.  In this
//  case the linked list node already exists, so a new node is not added to the list.  Hopfully at this time
//  everything is working again and AzureIoT_SendTelemetryCallback() will be called informing the application
//  that the message was successfully sent, at which time the node will be removed from the list.
//
//  Things to consider when using this functionality/feature
//
//  1. Each time a new node is added to the list memory is allocated.  If the application never reconnects
//     eventually the device will run out of memory.  Consider catching this condition and writting any 
//     pending telemetry data to persistant memory so that the telemetry could be sent after the application
//     restarts.  Currently if memory for a new node can't be allocated, the application will exit with reason
//     code ExitCode_AddTelemetry_Malloc_Failed.
//  
//  2. If the telemetry is re-sent, then there is no guarentee or controll mechanism to define how long after 
//     the first attempt the resend will occur.  If your cloud implementation is sensitive to time, then
//     consider adding a timestamp to your telemety message as an additional {"key": Value} entry.  The
//     implementation DOES resend the messages in the same order that they we're originally sent.
//     
//////////////////////////////////////////////////////////////////////////////////////////////////

//#define ENABLE_TELEMETRY_RESEND_LOGIC

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Optional connection to real-time M4 application
//
//  M4_INTERCORE_COMMS: Enable to add Intercore Communication code to the project
//  This will enable reading the ALST19 light sensor data from the M4 application
//  To exercise the inter-core communication code run the M4 application first
//
//////////////////////////////////////////////////////////////////////////////////////////////////

//#define M4_INTERCORE_COMMS

#ifdef M4_INTERCORE_COMMS
#define MAX_REAL_TIME_APPS 2
#define MAX_RT_MESSAGE_SIZE 256

// List of currently implemented Azure RTOS real time applications 
// define a max of two applications
//#define ENABLE_GROVE_GPS_RT_APP  // Read a Grove GPS UART sensor
//#define ENABLE_ALS_PT19_RT_APP     // Read the Starter Kit on-board light sensor
//#define ENABLE_GENERIC_RT_APP      // Example application that implements all the interfaces to work with this high level implementation
#endif 

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Default timer values
//
//////////////////////////////////////////////////////////////////////////////////////////////////


// Defines how often the read sensor periodic handler runs
#define SENSOR_READ_PERIOD_SECONDS 15
#define SENSOR_READ_PERIOD_NANO_SECONDS 0 * 1000

// Defines the default period to send telemetry data to the IoTHub
#define SEND_TELEMETRY_PERIOD_SECONDS 30
#define SEND_TELEMETRY_PERIOD_NANO_SECONDS 0 * 1000


//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Application/Device Constants
//
//  These items will be sent to the IoT Hub on connection as read only device twins
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#define VERSION_STRING "AvnetTemplate-V2" // {"versionString"; "AvnetTemplate-V2"}
#define DEVICE_MFG "Avnet" // {"manufacturer"; "Avnet"}
#define DEVICE_MODEL "Avnet Starter Kit" // {"model"; "Avnet Starter Kit"}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Debug control
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// Enables I2C read/write debug
//#define ENABLE_READ_WRITE_DEBUG

#endif 