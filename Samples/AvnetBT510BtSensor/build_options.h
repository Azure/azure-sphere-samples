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

#endif 