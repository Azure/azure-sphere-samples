/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

// If your application is going to connect straight to a IoT Hub or IoT Connect, then enable this define.
//#define IOT_HUB_APPLICATION

#if !defined(IOT_HUB_APPLICATION)
//#warning "Building application for no cloud connectivity"
#endif 

#ifdef IOT_HUB_APPLICATION
//#warning "Building for IoT Hub or IoT Central Application"
#endif 

// Define if you want to build the Azure IoT Hub/IoTCentral Plug and Play application functionality
//#define USE_PNP

// Make sure we're using the IOT Hub code for the PNP configuration
#ifdef USE_PNP
#define IOT_HUB_APPLICATION
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:avnet:mt3620Starterkit;1" // https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play 
#endif 

// Define to build for Avnet's IoT Connect platform
//#define USE_IOT_CONNECT

// If this is a IoT Conect build, make sure to enable the IOT Hub application code
#ifdef USE_IOT_CONNECT
#define IOT_HUB_APPLICATION
#endif 


// Include SD1306 OLED code
// To use the OLED 
// Install a 128x64 OLED display onto the unpopulated J7 Display connector
// https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
// Enable the OLED_SD1306 #define below
//#define OLED_SD1306

// Include Intercore Communication code
// This will enable reading the ALST19 light sensor data from the M4 application
// To exercise the inter-core communication code run the M4 application first
// Enable the M4_INTERCORE_COMMS #define below
//#define M4_INTERCORE_COMMS

// Defines how quickly the light sensor is read and reported from the M4 core
#define M4_READ_PERIOD_SECONDS 3
#define M4_READ_PERIOD_NANO_SECONDS 0 * 1000

// Defines how quickly the accelerator data is read and reported
#define SENSOR_READ_PERIOD_SECONDS 5
#define SENSOR_READ_PERIOD_NANO_SECONDS 0 * 1000

// Define how long after processing the haltApplication direct method before the application exits
#define HALT_APPLICATION_DELAY_TIME_SECONDS 5

// Enables I2C read/write debug
//#define ENABLE_READ_WRITE_DEBUG
#endif 