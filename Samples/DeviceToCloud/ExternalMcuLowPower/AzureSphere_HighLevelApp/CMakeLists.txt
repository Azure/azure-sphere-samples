#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(LowPowerMcuToCloud C)

azsphere_configure_tools(TOOLS_REVISION "21.04")
azsphere_configure_api(TARGET_API_SET "9")

add_executable(${PROJECT_NAME}
               main.c
               azure_iot.c
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
target_include_directories(${PROJECT_NAME} PRIVATE ../common)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c azureiot)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "soda_machine.json")

azsphere_target_add_image_package(${PROJECT_NAME})
