#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(ExternalMcuUpdateNrf52 C)

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c file_view.c mem_buf.c epoll_timerfd_utilities.c nordic/slip.c nordic/crc.c nordic/dfu_uart_protocol.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} applibs pthread gcc_s c)

# Add MakeImage post-build command
SET(ADDITIONAL_APPROOT_INCLUDES "ExternalNRF52Firmware/blinkyV1.bin;ExternalNRF52Firmware/blinkyV1.dat;ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.bin;ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.dat")
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
