#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(AzureIoT C)

azsphere_configure_tools(TOOLS_REVISION "22.02")
azsphere_configure_api(TARGET_API_SET "12")

add_executable(${PROJECT_NAME} "")

add_subdirectory(common)

# The subdirectory below can be set to one of IoTHub, DPS or IoTEdge, to include the code
# particular to that connectivity scenario.
add_subdirectory(IoTHub)

target_include_directories(${PROJECT_NAME} PUBLIC
                           ${CMAKE_SOURCE_DIR}/common
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_c_shared_utility)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} m azureiot applibs gcc_s c)

# TARGET_HARDWARE and TARGET_DEFINITION relate to the hardware definition targeted by this sample.
# When using this sample with other hardware, replace TARGET_HARDWARE with the name of that hardware.
# For example, to target the Avnet MT3620 Starter Kit, use the value "avnet_mt3620_sk".
# See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# This finds the path to the target hardware definition, in one of two places.
# If the sample is cloned from the github repo, then the path to the target hardware is 
# "../../HardwareDefinitions/${TARGET_HARDWARE}". If the sample is downloaded from the Microsoft
# samples browser, then the path is "HardwareDefinitions/${TARGET_HARDWARE}"
find_path(
    TARGET_DIRECTORY
    NAMES ${TARGET_DEFINITION}
    PATHS "HardwareDefinitions/${TARGET_HARDWARE}" "../../HardwareDefinitions/${TARGET_HARDWARE}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${TARGET_DIRECTORY}" TARGET_DEFINITION "${TARGET_DEFINITION}")

find_program(POWERSHELL powershell.exe)

if (POWERSHELL)
    # Run validate_manifest script during build
    add_custom_target(ValidateManifest ALL
        COMMAND ${POWERSHELL} -ExecutionPolicy Bypass -NoProfile -NonInteractive
        -File ${CMAKE_SOURCE_DIR}/script/validate_manifest.ps1
        -Manifest ${CMAKE_SOURCE_DIR}/app_manifest.json 
        -Helper ${MANIFEST_HELPER}
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
else()
    # Warn users without PowerShell to update their manifest
    add_custom_target(ValidateManifest ALL
        COMMAND echo "Please ensure that you have updated app_manifest.json as described in IoTCentral.md or IoTHub.md, as appropriate."
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
endif()

add_dependencies(ValidateManifest ${PROJECT_NAME})

azsphere_target_add_image_package(${PROJECT_NAME})
