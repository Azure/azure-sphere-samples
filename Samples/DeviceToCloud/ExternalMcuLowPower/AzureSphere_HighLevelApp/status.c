/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <applibs/gpio.h>
#include <applibs/log.h>

#include "status.h"

#include <hw/soda_machine.h>

static int statusLedRedGpioFd = -1;
static int statusLedGreenGpioFd = -1;
static int statusLedBlueGpioFd = -1;

static bool OpenStatusLeds(void);

void Status_NotifyStarting(void)
{
    if ((statusLedRedGpioFd == -1 || statusLedGreenGpioFd == -1 || statusLedBlueGpioFd == -1) &&
        !OpenStatusLeds()) {
        return;
    }

    GPIO_SetValue(statusLedGreenGpioFd, GPIO_Value_Low);
}

/// <summary>
/// Notify that the application is finished.
/// </summary>
void Status_NotifyFinished(void)
{
    if (statusLedRedGpioFd != -1) {
        GPIO_SetValue(statusLedRedGpioFd, GPIO_Value_High);
        close(statusLedRedGpioFd);
        statusLedRedGpioFd = -1;
    }

    if (statusLedGreenGpioFd != -1) {
        GPIO_SetValue(statusLedGreenGpioFd, GPIO_Value_High);
        close(statusLedGreenGpioFd);
        statusLedGreenGpioFd = -1;
    }

    if (statusLedBlueGpioFd != -1) {
        GPIO_SetValue(statusLedBlueGpioFd, GPIO_Value_High);
        close(statusLedBlueGpioFd);
        statusLedBlueGpioFd = -1;
    }
}

bool OpenStatusLeds(void)
{
    if (statusLedRedGpioFd == -1) {
        if ((statusLedRedGpioFd = GPIO_OpenAsOutput(
                 SODAMACHINE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High)) == -1) {
            Log_Debug("ERROR: Could not open status RGB red channel: %s (%d)\n", strerror(errno),
                      errno);
            return false;
        }
    }

    if (statusLedGreenGpioFd == -1) {
        if ((statusLedGreenGpioFd = GPIO_OpenAsOutput(
                 SODAMACHINE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High)) == -1) {
            Log_Debug("ERROR: Could not open status RGB green channel: %s (%d)\n", strerror(errno),
                      errno);
            return false;
        }
    }

    if (statusLedBlueGpioFd == -1) {
        if ((statusLedBlueGpioFd = GPIO_OpenAsOutput(
                 SODAMACHINE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High)) == -1) {
            Log_Debug("ERROR: Could not open status RGB blue channel: %s (%d)\n", strerror(errno),
                      errno);
            return false;
        }
    }

    return true;
}
