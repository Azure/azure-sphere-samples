CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(HTTPS_Curl_Multi C)

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c ui.c epoll_timerfd_utilities.c web_client.c log_utils.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} applibs pthread gcc_s c curl)

# Add MakeImage post-build command
SET(ADDITIONAL_APPROOT_INCLUDES "certs/bundle.pem")
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
