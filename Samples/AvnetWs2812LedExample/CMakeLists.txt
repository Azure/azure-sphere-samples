#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(GPIO_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "8") 

add_executable(${PROJECT_NAME} 
			main.c 
			eventloop_timer_utilities.c
			WS2812B_Driver/ws2812b.c
			WS2811_Driver/ws2811.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c) 

target_include_directories(${PROJECT_NAME} PUBLIC 
                "../ws2811_Driver" 
				"../ws2812B_Driver")


azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(${PROJECT_NAME})  
