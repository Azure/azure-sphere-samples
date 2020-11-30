#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(GPIO_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7") 

add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(${PROJECT_NAME})
