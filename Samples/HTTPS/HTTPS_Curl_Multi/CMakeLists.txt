#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(HTTPS_Curl_Multi C)

azsphere_configure_tools(TOOLS_REVISION "21.01")
azsphere_configure_api(TARGET_API_SET "8")

add_executable(${PROJECT_NAME} main.c ui.c eventloop_timer_utilities.c web_client.c log_utils.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl)

# Target hardware for the sample.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# The path to the target hardware for the sample.
# If the sample is downloaded from the repo, then the path to the target hardware is 
# "../../../HardwareDefinitions/${TARGET_HARDWARE}". If the sample is downloaded from the Microsoft
# samples browser, then the path is "HardwareDefinitions/${TARGET_HARDWARE}"
find_path(
    TARGET_DIRECTORY
    NAMES ${TARGET_DEFINITION}
    PATHS "HardwareDefinitions/${TARGET_HARDWARE}" "../../../HardwareDefinitions/${TARGET_HARDWARE}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${TARGET_DIRECTORY}" TARGET_DEFINITION "${TARGET_DEFINITION}")

azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/bundle.pem")
