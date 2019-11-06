/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This minimal Azure Sphere app repeatedly toggles GPIO 8, which is the red channel of RGB
// LED 1 on the MT3620 RDB. Use this app to test that device and SDK installation succeeded
// that you can build, deploy, and debug a CMake app with Visual Studio.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (messages shown in Visual Studio's Device Output window during debugging)

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed using the "AzureSphereTargetHardwareDefinitionDirectory" property in
// CMakeSettings.json (for Visual Studio), or the value passed to
// -DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY when invoking cmake from the command line.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

int main(void)
{
    Log_Debug("Starting CMake Hello World application...\n");

    int fd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (fd < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    const struct timespec sleepTime = {1, 0};
    while (true) {
        GPIO_SetValue(fd, GPIO_Value_Low);
        nanosleep(&sleepTime, NULL);
        GPIO_SetValue(fd, GPIO_Value_High);
        nanosleep(&sleepTime, NULL);
    }
}