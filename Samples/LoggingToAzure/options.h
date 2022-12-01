/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "exitcodes.h"

// This header defines an interface for options parsing.
// Implementations specific to each connection type (IoTHub, DPS and IoTEdge) can be found
// in the corresponding directory.

/// <summary>
/// Parse the provided options.
/// </summary>
/// <param name="argc">Number of arguments.</param>
/// <param name="argv">Array of null-terminated strings representing the arguments.</param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode Options_ParseArgs(int argc, char *argv[]);

/// <summary>
/// Get the context required for connection to Azure as specified by the provided options.
///
/// The context returned from this function should be passed to
/// <see cref="Connection_Initialize" />.
/// You must call <see cref="Options_ParseArgs" /> before calling this function.
/// </summary>
/// <returns>Implementation-specific connection context data.</returns>
void *Options_GetConnectionContext(void);
