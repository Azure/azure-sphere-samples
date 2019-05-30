/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

/// <summary>
/// Provides a movable window to a file's contents.
/// This removes the need to load the entire file into memory at once.
/// </summary>
typedef struct {
    /// <summary>
    /// Opened descriptor for current file.  This is owned by the file view.
    /// </summary>
    int fd;

    /// <summary>Size of window in bytes.</summary>
    size_t windowSize;

    /// <summary>Start of window in memory.</summary>
    uint8_t *window;

    /// <summary>Data in window starts at this offset in the file.</summary>
    off_t fileOffset;

    /// <summary>Total file size.</summary>
    off_t fileSize;
} FileView;

/// <summary>
/// Allocates a file view and opens the supplied file.  This function
/// does not load any part of the file into memory, so call FileViewMoveWindow
/// before attempting to read any data from the window.
/// <param name="path">Name of file to open.  This file must be in the image package.</param>
/// <param name="windowSize">Window size in bytes.</param>
/// <returns>On success, a pointer to a newly-allocated file view which the caller
/// must dispose of with CloseFileView.  On failure it returns NULL.</returns>
/// </summary>
FileView *OpenFileView(const char *path, size_t windowSize);

/// <summary>
/// Frees a file view which was allocated with OpenFileView.  It is safe to
/// call this function with a NULL pointer.
/// </summary>
void CloseFileView(FileView *self);

/// <summary>
/// Move the internal window so it starts at the supplied offset.
/// This function will read data up to the end of the window or the
/// end of the file, whichever is sooner.
/// <param name="self">File view returned by OpenFileView.</param>
/// <param name="offset">Offset in file from which to read data.</param>
/// <returns>true if successfully read data into the window; false otherwise.
/// If this function fails, then the state of the window is undefined and
/// the FileView object should be disposed of.</returns>
/// </summary>
bool FileViewMoveWindow(FileView *self, off_t offset);

/// <summary>
/// Gets current file offset and size.
/// <param name="self">File view returned by OpenFileView.</param>
/// <param name="offset">On return contains file offset.  This parameter can be NULL.</param>
/// <param name="size">On return contains file size.  This parameter can be NULL.</param>
///</summary>
void FileViewFileOffsetSize(const FileView *self, off_t *offset, off_t *size);

/// <summary>
/// Gets current window address and extent.
/// <param name="self">File view returned by OpenFileView.</param>
/// <param name="data">
///     On return contains start address of window.  This parameter can be NULL.
/// </param>
/// <param name="extent">On return contains size of window.</param>
///</summary>
void FileViewWindow(const FileView *self, uint8_t const **data, off_t *extent);
