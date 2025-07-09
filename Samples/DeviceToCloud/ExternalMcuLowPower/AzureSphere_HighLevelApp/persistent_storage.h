/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include "telemetry.h"

/// <summary>
///     Persist device telemetry to storage, for retrieval on a future run.
/// </summary>
/// <param name="telemetry">Pointer to a telemetry object to persist.</param>
void PersistentStorage_PersistTelemetry(const DeviceTelemetry *telemetry);

/// <summary>
///     Attempt to retrieve previously persisted device telemetry from storage. If no previous
///     telemetry can be found, returns false and sets all fields of the supplied telemetry object
///     to zero; otherwise, returns true and populates the supplied telemetry object.
/// </summary>
/// <param name="telemetry">
///     Pointer to a device telemetry object to receive the persisted data.
/// </param>
/// <returns>true if previously-persisted telemetry is found; false if not.</returns>
bool PersistentStorage_RetrieveTelemetry(DeviceTelemetry *telemetry);
