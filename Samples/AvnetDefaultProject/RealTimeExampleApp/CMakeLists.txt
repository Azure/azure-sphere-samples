#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)  

project(AvnetReferenceDesignRTApp C)
azsphere_configure_tools(TOOLS_REVISION "20.10") 

# Create executable
add_executable(${PROJECT_NAME} main.c mt3620-intercore.c mt3620-timer-poll.c mt3620-uart-poll.c mt3620-adc.c)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME}) 
  
