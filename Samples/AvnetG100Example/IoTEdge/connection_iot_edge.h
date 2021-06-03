/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
/// Context data required for connecting to an Azure IoT Edge device
/// </summary>
typedef struct {
    const char *iotEdgeCACertPath;
    const char *edgeDeviceHostname;
} Connection_IotEdge_Config;
