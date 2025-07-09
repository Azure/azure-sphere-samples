/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
///     Logs an error message including the current errno.
/// </summary>
void LogErrno(const char *message, ...);

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="error">Error message.</param>
void CloseFdAndLogOnError(int fd, const char *error);
