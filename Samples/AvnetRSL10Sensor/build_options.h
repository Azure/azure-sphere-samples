#ifndef build_options_h
#define build_options_h

// Enable to use the Ethernet interface
// Starter Kit Rev2 with an Eth Click
// Qiio 200 Cellular Guardian
//#define USE_ETH_0

// Enable if building for the Qiio-200 Development Board
//#define TARGET_QIIO_200

#ifdef TARGET_QIIO_200
#define USE_ETH_0
#endif 

//#define USE_IOT_CONNECT

// Include SD1306 OLED code
// To use the OLED 
// Install a 128x64 OLED display onto the unpopulated J7 Display connector
// https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
// Enable the OLED_SD1306 #define below
//#define OLED_SD1306

// Define the default behavior when new RSL10 devices are discovered
// If enabled, only authorized RSL10 devices will be allowed to send telemetry to Azure.  Authorizing devices
// is configured in the device twin.  Update authorizedMacN where N is 1 - 10.
// If disabled upto 10 RSL devices will be able to send telemetry.  Each telemetry message will include the mac
// address of the device that sent the telemety data.
//#define REQUIRE_AUTHORIZATION

// Define to build the Avnet Sales demo version of the application
//#define RSL10_SALES_DEMO

#ifdef RSL10_SALES_DEMO
#define USE_IOT_CONNECT

// Time between sending telemetry
#define DEFAULT_TELEMETRY_TX_TIME 2
#undef REQUIRE_AUTHORIZATION
#else

// Time between sending telemetry
#define DEFAULT_TELEMETRY_TX_TIME 30
#undef REQUIRE_AUTHORIZATION
#endif 

#endif 