/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
///     Initializes user interface resources.
/// </summary>
/// <param name="epollFdInstance">The epoll instance</param>
/// <returns>0 on success, or -1 on failure</returns>
int Ui_Init(int epollFdInstance);

/// <summary>
///     Finalizes user interface resources.
/// </summary>
void Ui_Fini(void);
