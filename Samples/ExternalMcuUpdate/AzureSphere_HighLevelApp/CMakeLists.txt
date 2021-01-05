#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(ExternalMcuUpdateNrf52 C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7")

add_executable(${PROJECT_NAME} main.c file_view.c mem_buf.c eventloop_timer_utilities.c nordic/slip.c nordic/crc.c nordic/dfu_uart_protocol.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

# Target hardware for the sample.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# The path to the target hardware for the sample.
# If the sample is downloaded from the repo, then the path to the target hardware is 
# "../../../HardwareDefinitions/${TARGET_HARDWARE}". If the sample is downloaded from the Microsoft
# samples browser, then the path is "HardwareDefinitions/${TARGET_HARDWARE}"
find_path(
    TARGET_DIRECTORY
    NAMES ${TARGET_DEFINITION}
    PATHS "HardwareDefinitions/${TARGET_HARDWARE}" "../../../HardwareDefinitions/${TARGET_HARDWARE}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${TARGET_DIRECTORY}" TARGET_DEFINITION "${TARGET_DEFINITION}")

azsphere_target_add_image_package(
    ${PROJECT_NAME}
    RESOURCE_FILES
        "ExternalNRF52Firmware/blinkyV1.bin"
        "ExternalNRF52Firmware/blinkyV1.dat"
        "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.bin"
        "ExternalNRF52Firmware/s132_nrf52_6.1.0_softdevice.dat")
