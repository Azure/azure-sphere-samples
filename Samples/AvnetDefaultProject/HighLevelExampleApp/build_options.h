/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Connectivity options
//
//  IOT_HUB_APPLICATION: Enable for any configuration that connects to an IoTHub/IoTCentral
//  USE_IOT_CONNECT: Enable to connect to Avnet's IoTConnect Cloud solution
//
//  USE_PNP: Enable to buid a PNP compatable application.  Note that the user must define, validate and 
//           publish the PnP model to Microsoft's GitHub repo
//           https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play                                    
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// If your application is going to connect straight to a IoT Hub or IoT Connect, then enable this define.
#define IOT_HUB_APPLICATION

// Define to build for Avnet's IoT Connect platform
//#define USE_IOT_CONNECT

// If this is a IoT Conect build, make sure to enable the IOT Hub application code
#ifdef USE_IOT_CONNECT
#define IOT_HUB_APPLICATION
#endif 

// Define if you want to build the Azure IoT Hub/IoTCentral Plug and Play application functionality
//#define USE_PNP

// Make sure we're using the IOT Hub code for the PNP configuration
#ifdef USE_PNP
#define IOT_HUB_APPLICATION
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:avnet:Starterkit;1" // https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play                                   
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

#define OLED_SD1306

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

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Default timer values
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// Defines how quickly the light sensor is read and reported from the M4 core
#define M4_READ_PERIOD_SECONDS 3
#define M4_READ_PERIOD_NANO_SECONDS 0 * 1000

// Defines how quickly the accelerator data is read and reported
#define SENSOR_READ_PERIOD_SECONDS 1
#define SENSOR_READ_PERIOD_NANO_SECONDS 0 * 1000

// Defines the default period to send telemetry data to the IoTHub
#define SEND_TELEMETRY_PERIOD_SECONDS 30
#define SEND_TELEMETRY_PERIOD_NANO_SECONDS 0 * 1000

// Define how long after processing the haltApplication direct method before the application exits
#define HALT_APPLICATION_DELAY_TIME_SECONDS 1

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Debug control
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// Enables I2C read/write debug
//#define ENABLE_READ_WRITE_DEBUG

#endif 