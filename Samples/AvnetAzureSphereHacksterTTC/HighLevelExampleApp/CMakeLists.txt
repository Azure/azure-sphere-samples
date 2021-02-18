#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project(AvnetReferenceDesign C)

azsphere_configure_tools(TOOLS_REVISION "21.01")
azsphere_configure_api(TARGET_API_SET "8")

set_source_files_properties(lsm6dso_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(lps22hh_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(i2c.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(oled.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(sd1306.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

add_executable(${PROJECT_NAME}  main.c 
                                eventloop_timer_utilities.c 
                                parson.c 
                                lps22hh_reg.c 
                                lsm6dso_reg.c 
                                i2c.c 
                                device_twin.c
                                oled.c
                                sd1306.c
                                iotConnect.c
                                direct_methods.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot 
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client 
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_c_shared_utility)

target_link_libraries(${PROJECT_NAME} m azureiot applibs gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")
#azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")

azsphere_target_add_image_package(${PROJECT_NAME})
