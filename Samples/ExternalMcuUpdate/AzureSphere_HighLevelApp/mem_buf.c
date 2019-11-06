/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// By default Azure Sphere Visual Studio-created applications define _POSIX_C_SOUCE  (in the vcxproj
// file). In this case we also depend on functions that libc provides which are not in POSIX, so we
// add _BSD_SOURCE
#define _BSD_SOURCE
#include <endian.h>

#include <applibs/log.h>
#include "mem_buf.h"

MemBuf *AllocMemBuf(size_t maxSize)
{
    uint8_t *data = calloc(maxSize, sizeof(uint8_t));
    if (!data) {
        return NULL;
    }

    MemBuf *self = malloc(sizeof(*self));
    if (!self) {
        free(data);
        return NULL;
    }

    self->maxSize = maxSize;
    self->curSize = 0;
    self->data = data;

    return self;
}

void FreeMemBuf(MemBuf *self)
{
    if (!self) {
        return;
    }

    free(self->data);
    free(self);
}

// ---- window management ----

void MemBufData(const MemBuf *self, uint8_t const **data, size_t *extent)
{
    if (data) {
        *data = self->data;
    }

    *extent = self->curSize;
}

size_t MemBufCurSize(const MemBuf *self)
{
    return self->curSize;
}

size_t MemBufMaxSize(const MemBuf *self)
{
    return self->maxSize;
}

void MemBufReset(MemBuf *self)
{
    self->curSize = 0;
}

bool MemBufResize(MemBuf *self, size_t maxSize)
{
    uint8_t *newData = realloc(self->data, maxSize);
    if (!newData) {
        return false;
    }

    self->data = newData;
    self->maxSize = maxSize;
    if (self->curSize > self->maxSize) {
        self->curSize = self->maxSize;
    }

    return true;
}

void MemBufShiftLeft(MemBuf *self, size_t distance)
{
    assert(distance <= self->curSize);

    size_t newSize = self->curSize - distance;
    memmove(self->data, &self->data[distance], newSize);
    self->curSize = newSize;
}

void MemBufDump(const MemBuf *self, const char *desc)
{
    size_t extent = self->curSize;
    Log_Debug("%s: (%zu): [", desc, extent);

    for (size_t i = 0; i < extent; ++i) {
        Log_Debug("%" PRIu8, MemBufRead8(self, i));
        if (i + 1 < self->curSize) {
            Log_Debug(" ");
        }
    }

    Log_Debug("]\n");
}

// ---- read / write window contents ----

void MemBufWrite8(MemBuf *self, size_t idx, uint8_t val)
{
    assert(idx < self->curSize);
    self->data[idx] = val;
}

uint8_t MemBufRead8(const MemBuf *self, size_t idx)
{
    assert(idx < self->curSize);
    return self->data[idx];
}

void MemBufAppend8(MemBuf *self, uint8_t val)
{
    assert(self->curSize < self->maxSize);

    ++self->curSize;
    MemBufWrite8(self, self->curSize - 1, val);
}

uint16_t MemBufReadLe16(const MemBuf *self, size_t offset)
{
    // Copy to a local value to avoid alignment problems.
    uint16_t value;
    memcpy(&value, &self->data[offset], sizeof(value));

    return le16toh(value);
}

uint32_t MemBufReadLe32(const MemBuf *self, size_t offset)
{
    // Copy to a local value to avoid alignment problems.
    uint32_t value;
    memcpy(&value, &self->data[offset], sizeof(value));

    return le32toh(value);
}
