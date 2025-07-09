/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This file defines constants used by both the MCU and the Azure Sphere application

/// <summary>
/// The capacity of the soda machine - after refill, there will be this many dispenses available
/// </summary>
#define MachineCapacity 5

/// <summary>
/// The number of dispenses remaining in the machine at which we consider ourselves to be in a "low
/// dispense" state - this will wake the MT3620, and signal to the IoT Central app that the machine
/// machine requires a refill.
/// </summary>
#define LowDispenseAlertThreshold 2
