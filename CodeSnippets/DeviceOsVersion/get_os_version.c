/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
// This code snippet demonstrates how to get the device OS version.

#include <applibs/log.h>
#include <applibs/applications.h>

struct Applications_OsVersion osVersion = { 0 };

int GetOsVersion(Applications_OsVersion* osVersion)
{
    if (Applications_GetOsVersion(osVersion) != 0) 
    {
        Log_Debug("Failed to get OS version");
        return -1;
    }

    Log_Debug("OS version %s\n", osVersion->version);
    return 0;
}
