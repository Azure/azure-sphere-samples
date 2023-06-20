#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(LoggingToAzure C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

add_executable(${PROJECT_NAME}
    applibs_versions.h
    azure_iot.c
    azure_iot.h
    log_azure.c
    log_azure.h
    connection.h
    eventloop_timer_utilities.c
    eventloop_timer_utilities.h
    exitcodes.h
    utils.c
    utils.h
    main.c
    options.h
    parson.c
    parson.h)

# The subdirectory below can be set to one of IoTHub, DPS or IoTEdge, to include the code
# particular to that connectivity scenario.
add_subdirectory(IoTHub)

target_include_directories(${PROJECT_NAME} PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_c_shared_utility)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} azureiot applibs gcc_s c)

azsphere_target_add_image_package(${PROJECT_NAME})
