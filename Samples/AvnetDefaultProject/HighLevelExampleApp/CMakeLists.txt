#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(AvnetTemplate C)

azsphere_configure_tools(TOOLS_REVISION "21.04")
azsphere_configure_api(TARGET_API_SET "9")

add_executable(${PROJECT_NAME} "")

# Include the source files in the avnet directory
add_subdirectory(avnet)

# Include the source files in the common directory
add_subdirectory(common)

# The subdirectory below can be set to one of IoTHub, DPS or IoTEdge, to include the code
# particular to each connectivity scenario.
add_subdirectory(DPS)
#add_subdirectory(IoTHub)
#add_subdirectory(IoTEdge)

target_include_directories(${PROJECT_NAME} PUBLIC
                           ${CMAKE_SOURCE_DIR}/common
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_c_shared_utility)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wno-conversion)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} m azureiot applibs gcc_s c)

# Target hardware for the sample.  Select the line that corresponds to your Avnet Kit and revision
#set(TARGET_HARDWARE "avnet_g100") # For Guardian 100 builds make sure to enable the GUARDIAN_100 build option in build_options.h
#set(TARGET_HARDWARE "avnet_mt3620_sk_rev2")
set(TARGET_HARDWARE "avnet_mt3620_sk")

set(TARGET_DEFINITION "sample_appliance.json")

find_path(
    TARGET_DIRECTORY
    NAMES ${TARGET_DEFINITION}
    PATHS "../../../HardwareDefinitions/${TARGET_HARDWARE}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${TARGET_DIRECTORY}" TARGET_DEFINITION "${TARGET_DEFINITION}")

azsphere_target_add_image_package(${PROJECT_NAME})
