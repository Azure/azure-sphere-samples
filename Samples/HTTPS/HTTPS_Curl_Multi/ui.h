/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>

#include "curlmulti.h"

/// <summary>
///     Initializes user interface resources.
/// </summary>
/// <param name="eventLoopInstance">The event loop instance.</param>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
ExitCode Ui_Init(EventLoop *eventLoopInstance);

/// <summary>
///     Finalizes user interface resources.
/// </summary>
void Ui_Fini(void);
