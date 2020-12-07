#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(arducam-mini-plus C)  

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7")

add_executable(${PROJECT_NAME} 
                main.c 
                eventloop_timer_utilities.c
                arducam_driver/ArduCAM.c 
                arducam_driver/ll.c 
                arducam_driver/ov2640_regs.c 
                arducam.c
                delay.c) 

target_link_libraries(${PROJECT_NAME}
                applibs 
                gcc_s 
                c
                curl)

# Set the defines for the build
target_compile_definitions(${PROJECT_NAME} PUBLIC
            AzureSphere_CA7
            CFG_MODE_JPEG
            USE_OV5642)      

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")
 
# include the cert into the image package
azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/BaltimoreCyberTrustRoot.pem")
