/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <endian.h>

/// <summary>
/// <para>An in-memory buffer which is used to store encoded data before it is
/// written to the UART, and to store decoded data which is read from the
/// UART.<para>
/// <para>The buffer's maximum size is set when it is allocated or resized, but
/// the caller does not have to use the whole buffer.  The buffer will track the
/// amount of space which is currently used.
/// </summary>
typedef struct {
    /// <summary>Maximum size of buffer in bytes.</summary>
    size_t maxSize;

    /// <summary>Current size of buffer in bytes.</summary>
    size_t curSize;

    /// <summary>Start of buffer in memory.</summary>
    uint8_t *data;
} MemBuf;

/// <summary>
/// <para>Allocate a new buffer.</para>
/// <para>On success the buffer is empty and any unused contents are
/// undefined.</para>
/// <param name="maxSize">Maximum buffer size in bytes.</param>
/// <returns>Newly-allocated buffer, which must be disposed of with FreeMemBuf.
/// On failure returns NULL.</returns>
/// </summary>
MemBuf *AllocMemBuf(size_t maxSize);

/// <summary>
/// Frees a memory buffer which was allocated with AllocMemBuf.
/// <param name="self">Buffer which was allocated by AllocMemBuf.  It is safe
/// to call this function with a NULL pointer.</param>
/// </summary>
void FreeMemBuf(MemBuf *self);

/// <summary>
/// Get address and extent of data in buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="data">On return contains address of data.  This parameter
/// can be NULL.</param>
/// <param name="extent">On return contains amount of used buffer space in bytes.</param>
/// </summary>
void MemBufData(const MemBuf *self, uint8_t const **data, size_t *extent);

/// <summary>
/// Gets the current buffer size in bytes.  This is more convenient than
/// calling MemBufData with a pointer to a variable.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <returns>Amount of used buffer space in bytes.</returns>
/// </summary>
size_t MemBufCurSize(const MemBuf *self);

/// <summary>
/// Gets the msximum buffer size in bytes.  This is more convenient than
/// calling MemBufData with a pointer to a variable.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <returns>Maximum buffer size in bytes.</returns>
/// </summary>
size_t MemBufMaxSize(const MemBuf *self);

/// <summary>
/// Sets the current buffer size to zero.  Does not free the buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// </summary>
void MemBufReset(MemBuf *self);

/// <summary>
/// Changes the maximum buffer size.  Any existing data will be
/// preserved if possible.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="maxSize">New maximum size in bytes.</param>
/// <returns>true if the buffer was resized; false otherwise.  If the
/// buffer was not successfully resized then the current size and contents
/// are unchanged.</returns>
/// </summary>
bool MemBufResize(MemBuf *self, size_t maxSize);

/// <summary>
/// Discards data at the beginning of the buffer and moves the following
/// data down.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="distance">Number of bytes to discard from the start
/// of the buffer.  This must be no greater than the current buffer size.</param>
/// </summary>
void MemBufShiftLeft(MemBuf *self, size_t distance);

/// <summary>
/// Provided for debugging purposes, writes the buffer contents
/// to stdout.  Each byte is printed as a decimal integer and the
/// sequence is surrounded by square brackets.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="desc">A string to print before the contents.</param>
/// </summary>
void MemBufDump(const MemBuf *self, const char *desc);

/// <summary>
/// Writes an unsigned 8-bit value into the buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="idx">Offset at which to write the value.  This must be
/// less than the current buffer size.</param>
/// <param name="val">Value to write into the buffer.</param>
/// </summary>
void MemBufWrite8(MemBuf *self, size_t idx, uint8_t val);

/// <summary>
/// Reads an unsigned 8-bit value from the buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="idx">Offset from which to read the value.  This must be
/// less than the current buffer size.</param>
/// <returns>Unsigned 8-bit value at supplied index.</returns>
/// </summary>
uint8_t MemBufRead8(const MemBuf *self, size_t idx);

/// <summary>
/// <para>Append an 8-bit value to the end of the buffer.</para>
/// <para>On exit the current size is increased by one.  It must not
/// exceed the maximum size.</para>
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="val">Value to append to the end of the buffer.</param>
/// </summary>
void MemBufAppend8(MemBuf *self, uint8_t val);

/// <summary>
/// Read a unsigned little-endian 16-bit value from the buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="offset">Offset from which to read the value.  The
/// entire value must fit within the current buffer size.</param>
/// <returns>Unsigned 16-bit value at supplied offset.</returns>
/// </summary>
uint16_t MemBufReadLe16(const MemBuf *self, size_t offset);

/// <summary>
/// Read a unsigned little-endian 32-bit value from the buffer.
/// <param name="self">Buffer which was allocated by AllocMemBuf.</param>
/// <param name="offset">Offset from which to read the value.  The
/// entire value must fit within the current buffer size.</param>
/// <returns>Unsigned 32-bit value at supplied offset.</returns>
/// </summary>
uint32_t MemBufReadLe32(const MemBuf *self, size_t offset);
