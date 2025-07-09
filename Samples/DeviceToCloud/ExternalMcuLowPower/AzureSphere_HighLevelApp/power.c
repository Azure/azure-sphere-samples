/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>

#include <applibs/powermanagement.h>

#include <applibs/log.h>

const unsigned int powerdownResidencyTimeSeconds = 120;

void Power_RequestPowerdown(void)
{
    if (PowerManagement_ForceSystemPowerDown(powerdownResidencyTimeSeconds) != 0) {
        Log_Debug("ERROR: Unable to force a system power down: %s (%d).\n", strerror(errno), errno);
    } else {
        Log_Debug("INFO: System power down requested.\n");
    }
}

void Power_RequestReboot(void)
{
    if (PowerManagement_ForceSystemReboot() != 0) {
        Log_Debug("ERROR: Unable to force a system reboot. %s (%d).\n", strerror(errno), errno);
    } else {
        Log_Debug("INFO: System reboot requested.\n");
    }
}

void Power_SetPowerSaveMode(void)
{
    if (PowerManagement_SetSystemPowerProfile(PowerManagement_PowerSaver) != 0) {
        Log_Debug("ERROR: Unable to set power-save profile: %s (%d).\n", strerror(errno), errno);
    } else {
        Log_Debug("INFO: Power-saver profile enabled.\n");
    }
}
