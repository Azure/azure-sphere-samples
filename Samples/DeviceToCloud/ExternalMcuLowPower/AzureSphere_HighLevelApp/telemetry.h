/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>

/// <summary>
/// Defines the version of the telemetry struct; increment if the struct below is modified.
/// </summary>
static const uint32_t telemetryStructVersion = 2u;

/// <summary>
///     Telemetry read from the device.
/// </summary>
typedef struct {
    /// <summary>
    /// Accumulated total number of dispenses made by the machine (since first run)
    /// </summary>
    uint32_t lifetimeTotalDispenses;

    /// <summary>
    /// Accumulated total number of dispenses stocked in the machine (since first run)
    /// </summary>
    uint32_t lifetimeTotalStockedDispenses;

    /// <summary>
    /// Maximum number of dispenses that can be stocked at once
    /// </summary>
    uint32_t capacity;

    /// <summary>
    ///     Current battery level (V)
    /// </summary>
    float batteryLevel;
} DeviceTelemetry;

/// <summary>
///     Telemetry for sending to the cloud.
/// </summary>
typedef struct {
    /// <summary>
    /// Accumulated total number of dispenses made by the machine (since first run)
    /// </summary>
    uint32_t lifetimeTotalDispenses;

    /// <summary>
    /// Number of dispenses since last sync with the cloud backend
    /// </summary>
    uint32_t dispensesSinceLastSync;

    /// <summary>
    /// Point-in-time snapshot of remaining dispenses
    /// </summary>
    uint32_t remainingDispenses;

    /// <summary>
    /// Boolean indicating if the machine is running low on soda
    /// </summary>
    bool lowSoda;

    /// <summary>
    ///     Current battery level (V)
    /// </summary>
    float batteryLevel;
} CloudTelemetry;
