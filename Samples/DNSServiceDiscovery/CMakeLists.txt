#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(DNSServiceDiscovery C)

azsphere_configure_tools(TOOLS_REVISION "22.09")
azsphere_configure_api(TARGET_API_SET "14")

add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c dns-sd.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

azsphere_target_add_image_package(${PROJECT_NAME})
