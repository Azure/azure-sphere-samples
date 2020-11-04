#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(AzureSpherePOC C)

azsphere_configure_tools(TOOLS_REVISION "20.07")
azsphere_configure_api(TARGET_API_SET "6")

add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c parson.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} m azureiot applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")

find_program(POWERSHELL powershell.exe)

if (POWERSHELL)
    # Run validate_manifest script during build
    add_custom_target(ValidateManifest ALL
        COMMAND ${POWERSHELL} -ExecutionPolicy Bypass -NoProfile -NonInteractive -File ${CMAKE_SOURCE_DIR}/script/validate_manifest.ps1
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
else()
    # Warn users without PowerShell to update their manifest
    add_custom_target(ValidateManifest ALL
        COMMAND echo "Please ensure that you have updated app_manifest.json as described in IoTCentral.md or IoTHub.md, as appropriate."
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
endif()

add_dependencies(ValidateManifest ${PROJECT_NAME})

azsphere_target_add_image_package(${PROJECT_NAME})
