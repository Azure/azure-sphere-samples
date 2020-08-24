/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>

#include "exitcode.h"

typedef void (*Update_UpdatesCompleteCallback)(bool rebootRequired);

/// <summary>
///     Initialize update and powerdown handling. Once called, any pending updates will be deferred
///     until <see cref="Update_NotifyBusinessLogicComplete" /> is called.
///     Once any updates have been completed, <paramref name="updateCompleteCallback" /> is
///     invoked, indicating whether a reboot is required. In the event of a fatal error,
///     <paramref name="failureCallback" /> is invoked with a suitable exit code.
/// </summary>
/// <param name="el">EventLoop to register events and timers.</param>
/// <param name="updateCompleteCallback">
///     Function to be invoked when update processing is complete.
/// </param>
/// <param name="updateCompleteCallback">
///     Function to be invoked when a fatal error occurs during update check.
/// </param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode Update_Initialize(EventLoop *el, Update_UpdatesCompleteCallback updateCompleteCallback,
                           ExitCodeCallbackType failureCallback);

/// <summary>
///     Clean up update handling.
/// </summary>
void Update_Cleanup(void);

/// <summary>
///     Indicate to update handling that business logic is complete (and so pending updates
///     can now be installed)
/// </summary>
void Update_NotifyBusinessLogicComplete(void);
