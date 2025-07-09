/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_public.h"

/// <summary>
///     Initialize the Wi-Fi configuration message protocol by registering callback handlers
///     and setting up internal state.
/// </summary>
void WifiConfigMessageProtocol_Init(void);

/// <summary>
///     Clean up the Wi-Fi configuration message protocol callback handlers and internal state.
/// </summary>
void WifiConfigMessageProtocol_Cleanup(void);
