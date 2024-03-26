#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(IntercoreComms_RTApp_MT3620_BareMetal C)

add_executable(${PROJECT_NAME} main.c logical-intercore.c logical-dpc.c mt3620-intercore.c mt3620-uart-poll.c mt3620-timer.c)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_target_add_image_package(${PROJECT_NAME})

