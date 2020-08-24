#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(WifiSetupAndDeviceControlViaBle C)

azsphere_configure_tools(TOOLS_REVISION "20.07")
azsphere_configure_api(TARGET_API_SET "6")

add_executable(${PROJECT_NAME} main.c wificonfig_message_protocol.c blecontrol_message_protocol.c devicecontrol_message_protocol.c message_protocol.c epoll_timerfd_utilities.c ../common/message_protocol_utilities.c)
target_include_directories(${PROJECT_NAME} PUBLIC ../common)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(${PROJECT_NAME})
