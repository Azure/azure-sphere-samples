/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_INTERCORE_H
#define MT3620_INTERCORE_H

#include <stdint.h>

/// <summary>
/// There are two buffers, inbound and outbound, which are used to track
/// how much data has been written to, and read from, each shared buffer.
/// </summary>
typedef struct {
    /// <summary>
    /// <para>Enqueue function uses this value to store the last position written to
    /// by the real-time capable application.</para>
    /// <para>Dequeue function uses this value to find the last position written to by
    /// the high-level application.</summary>
    uint32_t writePosition;
    /// <summary>
    /// <para>Enqueue function uses this value to find the last position read from by the
    /// high-level applicaton.</para>
    /// <para>Dequeue function uses this value to store the last position read from by
    /// the real-time application.</para>
    uint32_t readPosition;
    /// <summary>Reserved for alignment.</summary>
    uint32_t reserved[14];
} BufferHeader;

/// <summary>Blocks inside the shared buffer have this alignment.</summary>
#define RINGBUFFER_ALIGNMENT 16

/// <summary>
/// <para>Gets the inbound and outbound buffers used to communicate with the high-level
/// application.  This function blocks until that data is available from the mailbox.</para>
/// <para>The retrieved pointers are then supplied to <see cref="EnqueueData" /> and
/// <see cref="DequeueData" />.</para>
/// </summary>
/// <param name="outbound">On success, this points to the buffer which the real-time capable
/// application uses to send messages to the high-level application.</param>
/// <param name="inbound">On success, this points to the buffer which the real-time capable
/// application uses to receive messages from the high-level application.</param>
/// <param name="bufSize">On success, this contains the buffer size in bytes.</param>
/// <returns>0 on success, -1 on failure.</returns>
int GetIntercoreBuffers(BufferHeader **outbound, BufferHeader **inbound, uint32_t *bufSize);

/// <summary>
/// Add data to the shared buffer, to be read by the high-level application.
/// </summary>
/// <param name="outbound">The outbound buffer, as obtained from <see cref="GetIntercoreBuffers" />.
/// </param>
/// <param name="inbound">The inbound buffer, as obtained from <see cref="GetIntercoreBuffers" />.
/// </param>
/// <param name="bufSize">
/// The total buffer size, as obtained from <see cref="GetIntercoreBuffers" />.
/// </param>
/// <param name="src">Start of data to write to buffer.</param>
/// <param name="dataSize">Length of data to write to buffer in bytes.</param>
/// <returns>0 if able to enqueue the data, -1 otherwise.</returns>
int EnqueueData(BufferHeader *inbound, BufferHeader *outbound, uint32_t bufSize, const void *src,
                uint32_t dataSize);

/// <summary>
/// Remove data from the shared buffer, which has been written by the high-level application.
/// </summary>
/// <param name="outbound">The outbound buffer, as obtained from <see cref="GetIntercoreBuffers" />.
/// </param>
/// <param name="inbound">The inbound buffer, as obtained from <see cref="GetIntercoreBuffers" />.
/// </param>
/// <param name="bufSize">Total size of shared buffer in bytes.</param>
/// <param name="dest">Data from the shared buffer is copied into this buffer.</param>
/// <param name="dataSize">On entry, contains maximum size of destination buffer in bytes.
/// On exit, contains the actual number of bytes which were written to the destination buffer.
/// </param>
/// <returns>0 if able to dequeue the data, -1 otherwise.</returns>
int DequeueData(BufferHeader *outbound, BufferHeader *inbound, uint32_t bufSize, void *dest,
                uint32_t *dataSize);

#endif // #ifndef MT3620_INTERCORE_H
