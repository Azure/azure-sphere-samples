#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(HTTPS_Curl_Multi C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7")

add_executable(${PROJECT_NAME} main.c ui.c eventloop_timer_utilities.c web_client.c log_utils.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/bundle.pem")
