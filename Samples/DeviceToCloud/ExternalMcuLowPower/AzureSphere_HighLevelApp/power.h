/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
///     Request that the device powers down for a period.
/// </summary>
void Power_RequestPowerdown(void);

/// <summary>
///     Request that the device reboots.
/// </summary>
void Power_RequestReboot(void);

/// <summary>
///     Request the device is put into power save mode.
/// </summary>
void Power_SetPowerSaveMode(void);
