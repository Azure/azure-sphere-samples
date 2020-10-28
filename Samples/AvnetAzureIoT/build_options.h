#pragma once

// If your application is going to connect straight to a IoT Hub, then enable this define.
//#define IOT_HUB_APPLICATION

#ifdef IOT_HUB_APPLICATION
#warning "Building for IoT Hub Application"
#endif