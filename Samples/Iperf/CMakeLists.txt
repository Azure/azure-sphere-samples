# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(iperf3 C)

# Create executable
add_executable(${PROJECT_NAME}
    iperf/src/main.c
    iperf/src/cjson.c
    iperf/src/dscp.c
    iperf/src/iperf_client_api.c
    iperf/src/iperf_error.c
    iperf/src/iperf_locale.c
    iperf/src/iperf_sctp.c
    iperf/src/iperf_server_api.c
    iperf/src/iperf_tcp.c
    iperf/src/iperf_time.c
    iperf/src/iperf_udp.c
    iperf/src/net.c
    iperf/src/tcp_info.c
    iperf/src/timer.c
    iperf/src/units.c

    overrides/iperf_api.c
    overrides/iperf_util.c)

add_compile_definitions(_GNU_SOURCE)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/iperf/src ${CMAKE_CURRENT_SOURCE_DIR}/overrides)

azsphere_target_add_image_package(${PROJECT_NAME})
