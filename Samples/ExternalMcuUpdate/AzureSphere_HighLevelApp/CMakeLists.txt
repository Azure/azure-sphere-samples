#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(ExternalMcuUpdateNrf52 C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7")

add_executable(${PROJECT_NAME} main.c file_view.c mem_buf.c eventloop_timer_utilities.c nordic/slip.c nordic/crc.c nordic/dfu_uart_protocol.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(
    ${PROJECT_NAME}
    RESOURCE_FILES
        "ExternalNRF52Firmware/blinkyV1.bin"
        "ExternalNRF52Firmware/blinkyV1.dat"
        "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.bin"
        "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.dat")
