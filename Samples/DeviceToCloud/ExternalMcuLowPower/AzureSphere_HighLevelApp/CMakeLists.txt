#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(LowPowerMcuToCloud C)

add_executable(${PROJECT_NAME}
               main.c
               azure_iot/azure_iot.c
               azure_iot/connection_dps.c
               business_logic.c
               cloud.c
               color.c
               debug_uart.c
               eventloop_timer_utilities.c
               logging.c
               message_protocol.c
               mcu_messaging.c
               persistent_storage.c
               parson.c
               power.c
               status.c
               uart_transport.c
               update.c
               ../common/message_protocol_utilities.c)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
target_include_directories(${PROJECT_NAME} PRIVATE ../common ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/azure_iot ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c azureiot)

# Target hardware for the sample.
# See https://learn.microsoft.com/azure-sphere/hardware/hardware-abstraction for further details on this feature.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "soda_machine.json")

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/${TARGET_HARDWARE}" TARGET_DEFINITION "${TARGET_DEFINITION}")

azsphere_target_add_image_package(${PROJECT_NAME})

