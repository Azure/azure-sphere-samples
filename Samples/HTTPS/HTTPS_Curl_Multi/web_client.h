/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "exitcode_curlmulti.h"

/// <summary>
///     Initializes the web client's resources.
/// </summary>
/// <param name="epollFdInstance">The epoll instance</param>
/// <returns>0 on success, -1 on error</returns>
ExitCode WebClient_Init(int epollFdInstance);

/// <summary>
///     Finalizes the web client's resources.
/// </summary>
void WebClient_Fini(void);

/// <summary>
///     Starts web page download.
/// </summary>
/// <returns>0 on success, -1 on error</returns>
int WebClient_StartTransfers(void);