/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "curlmulti.h"

/// <summary>
///     Initializes the web client's resources.
/// </summary>
/// <param name="eventLoopInstance">Event loop which is used to handle socket IO.</param>
/// <param name="bypassProxy">Boolean which indicates if proxy needs to be bypassed.</param>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
ExitCode WebClient_Init(EventLoop *eventLoopInstance, bool bypassProxy);

/// <summary>
///     Finalizes the web client's resources.
/// </summary>
void WebClient_Fini(void);

/// <summary>
///     Starts web page download.
/// </summary>
/// <returns>0 on success, -1 on error</returns>
int WebClient_StartTransfers(void);