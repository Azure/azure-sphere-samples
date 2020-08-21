/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>

#include <applibs/eventloop.h>

#include "color.h"
#include "exitcode.h"

/// <summary>
///     Initialize the business logic for the application
/// </summary>
ExitCode BusinessLogic_Initialize(EventLoop *el);

/// <summary>
///     Run the business logic for the application - this should be regularly polled until the
///     return value indicates that the business logic is complete. On completion (successful or
///     otherwise), <paramref name="ec" /> will be set as appropriate.
/// </summary>
/// <param name="ec">
///     Pointer to an ExitCode; will be set if the business logic is complete, otherwise
///     the value is undefined.
/// </param>
/// <returns>
///     A boolean indicating whether the business logic is complete (successfully or otherwise).
/// </returns>
bool BusinessLogic_Run(ExitCode *ec);

/// <summary>
///     Notify the business logic that the check for updates has completed.
/// </summary>
/// <param name="rebootRequired">
///     Indicates if a reboot is required following the update check and install.
/// </param>
void BusinessLogic_NotifyUpdateCheckComplete(bool rebootRequired);

/// <summary>
///     Notify the business logic that the check for updates has failed. This will allow the
///     business logic to complete, if possible; on completion of the business logic, the exit code
///     will be returned.
/// </summary>
/// <param name="businessLogicExitCode">
///     Exit code specifying the part of the update check that has failed.
/// </param>
void BusinessLogic_NotifyUpdateCheckFailed(ExitCode exitCode);

/// <summary>
///     Notify the business logic that the cloud connectivity state has changed.
/// </summary>
/// <param name="connected">
///     A Boolean indicating whether a connection to the cloud backend is available.
/// </param>
void BusinessLogic_NotifyCloudConnectionChange(bool connected);

/// <summary>
///     Notify the business logic that a request for a flavor change has been received from the
///     cloud.
/// </summary>
/// <param name="color">LED color associated with this flavor.</param>
/// <param name="flavorName">Flavor name.</param>
void BusinessLogic_NotifyCloudFlavorChange(const LedColor *color, const char *flavorName);

/// <summary>
///     Notify the business logic that an unrecoverable error has occurred. This will cause the
///     business logic to halt, but wait for any pending update check to complete, before shutting
///     down or rebooting as appropriate.
/// </summary>
/// <param name="businessLogicExitCode">
///     Exit code describing the fatal error.
/// </param>
void BusinessLogic_NotifyFatalError(ExitCode exitCode);
