/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "message_protocol_private.h"
#include <stdbool.h>

/// <summary>
///     Check if the provided message data is complete.
/// </summary>
/// <param name="message">The message to check.</param>
/// <param name="total_bytes_to_send">The size of the message in bytes.</param>
/// <returns>true if the message is complete, false otherwise.</returns>
bool MessageProtocol_IsMessageComplete(uint8_t *message, uint8_t messageLength);
