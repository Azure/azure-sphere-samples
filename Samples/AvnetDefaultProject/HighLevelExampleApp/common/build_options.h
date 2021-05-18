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
//#define USE_PNP

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