/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <applibs/log.h>
#include <applibs/storage.h>

#include "file_view.h"

// Call FileViewMoveWindow before attempting to read data from the window.
// This special value means that the file view does not contain valid data.
static const off_t NO_VALID_WINDOW = -1;

FileView *OpenFileView(const char *path, size_t windowSize)
{
    FileView *self = malloc(sizeof(*self));
    if (!self) {
        return NULL;
    }

    // Initialize owned resources so they can be cleaned
    // up safely if only some of them are initialized.
    self->fd = -1;
    self->fileOffset = NO_VALID_WINDOW;
    self->window = NULL;

    self->windowSize = windowSize;
    self->window = malloc(windowSize);
    if (!self->window) {
        goto failed;
    }

    self->fd = Storage_OpenFileInImagePackage(path);
    if (self->fd == -1) {
        goto failed;
    }

    self->fileSize = lseek(self->fd, 0, SEEK_END);
    if (self->fileSize == -1) {
        goto failed;
    }

    return self;

failed:
    CloseFileView(self);
    return NULL;
}

void CloseFileView(FileView *self)
{
    if (!self) {
        return;
    }

    if (self->fd != -1) {
        close(self->fd);
    }

    free(self->window);
    free(self);
}

bool FileViewMoveWindow(FileView *self, off_t offset)
{
    if (lseek(self->fd, offset, SEEK_SET) == -1) {
        Log_Debug("ERROR:%s: could not seek to %lld (errno=%d)\n", __func__, offset, errno);
        return false;
    }

    // Read up to the end of the window or up to the end of
    // the file, whichever is sooner.
    off_t bytesToRead = self->fileSize - offset;
    if (bytesToRead > (off_t)self->windowSize) {
        bytesToRead = self->windowSize;
    }

    off_t bytesSoFar = 0;
    while (bytesSoFar < bytesToRead) {
        off_t remainBytes = bytesToRead - bytesSoFar;
        int b = read(self->fd, &self->window[bytesSoFar], (size_t)remainBytes);
        if (b == -1) {
            Log_Debug("ERROR:%s: read failure bytes_so_far=%lld, remain_bytes=%lld, errno=%d\n",
                      __func__, bytesSoFar, remainBytes, errno);
            return false;
        }
        bytesSoFar += b;
    }

    self->fileOffset = offset;
    return true;
}

void FileViewFileOffsetSize(const FileView *self, off_t *offset, off_t *size)
{
    if (offset != 0) {
        *offset = self->fileOffset;
    }

    if (size != 0) {
        *size = self->fileSize;
    }
}

void FileViewWindow(const FileView *self, uint8_t const **data, off_t *extent)
{
    assert(self->fileOffset != NO_VALID_WINDOW);

    if (data) {
        *data = self->window;
    }

    off_t availBytes = self->windowSize;
    if (self->fileOffset + availBytes > self->fileSize) {
        availBytes = self->fileSize - self->fileOffset;
    }

    *extent = availBytes;
}
