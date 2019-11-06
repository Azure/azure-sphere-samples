#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(WifiSetupAndDeviceControlViaBle C)

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c wificonfig_message_protocol.c blecontrol_message_protocol.c devicecontrol_message_protocol.c message_protocol.c epoll_timerfd_utilities.c ../common/message_protocol_utilities.c)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ../common)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} applibs pthread gcc_s c)

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
