/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "logical-intercore.h"

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"

/// <summary>
///     The inbound and outbound buffers track how much data has been written
///     written to, and read from, each shared buffer.
/// </summary>
struct BufferHeaderImpl {
    /// <summary>
    ///     <para>
    ///         <see cref="IntercoreSend" /> uses this value to store the last position written to
    ///         by the real-time capable application.
    ///     </para>
    ///     <para>
    ///         <see cref="IntercoreRecv" /> uses this value to find the last position
    ///         written to by the high-level application.
    ///     </para>
    /// </summary>
    uint32_t writePosition;
    /// <summary>
    ///     <para>
    ///         <see cref="IntercoreSend" /> uses this value to find the last position read from by
    ///         the high-level application.
    ///     </para>
    ///     <para>
    ///         <see cref="IntercoreRecv" /> uses this value to store the last position read from by
    ///         the real-time capable application.
    ///     </para>
    /// </summary>
    uint32_t readPosition;
    /// <summary>Align up to 64 bytes, to match high-level L2 cache line.</summary>
    uint32_t reserved[14];
};

static uint32_t GetBufferSize(uint32_t bufferBase);
static BufferHeader *GetBufferHeader(uint32_t bufferBase);

static uint8_t *DataAreaOffset8(BufferHeader *header, uint32_t offset);
static uint32_t RoundUp(uint32_t value, uint32_t alignment);

static uint32_t ReadInboundCircular(const IntercoreComm *icc, uint32_t startPos, void *dest,
                                    size_t len);
static uint32_t WriteOutboundCircular(const IntercoreComm *icc, uint32_t startPos, const void *src,
                                      size_t size);

// If intercore debugging is enabled and the application detects a corrupt buffer,
// it will spin forever in the Assert function. The user can then use a debugger
// to see the type of corruption was detected.

#define DEBUG_INTERCORE
#ifdef DEBUG_INTERCORE

static void Assert(bool cond)
{
    if (cond) {
        return;
    }

    for (;;) {
        // empty.
    }
}

#define INTERCORE_ASSERT(c) Assert(c)

#else

#define INTERCORE_ASSERT(c)

#endif

// The buffer size is encoded as a power of two in the bottom five bits.
static uint32_t GetBufferSize(uint32_t bufferBase)
{
    return (UINT32_C(1) << (bufferBase & 0x1F));
}

// The buffer header is a pointer is a 32-byte aligned pointer which is
// stored in the top 27 bits.
static BufferHeader *GetBufferHeader(uint32_t bufferBase)
{
    return (BufferHeader *)(bufferBase & ~0x1F);
}

IntercoreResult SetupIntercoreComm(IntercoreComm *icc, Callback recvCallback)
{
    uint32_t inboundBase, outboundBase;
    MT3620_SetupIntercoreComm(&inboundBase, &outboundBase, recvCallback);

    uint32_t totalInboundBufSize = GetBufferSize(inboundBase);
    uint32_t totalOutboundBufSize = GetBufferSize(outboundBase);

    INTERCORE_ASSERT(totalInboundBufSize > sizeof(BufferHeader));
    INTERCORE_ASSERT(totalOutboundBufSize > sizeof(BufferHeader));

    // Reduce buffer sizes to exclude headers.
    icc->inboundBufSize = totalInboundBufSize - sizeof(BufferHeader);
    icc->outboundBufSize = totalOutboundBufSize - sizeof(BufferHeader);

    icc->inbound = GetBufferHeader(inboundBase);
    icc->outbound = GetBufferHeader(outboundBase);

    return Intercore_OK;
}

// Converts offset into shared buffer into a memory pointer.
static uint8_t *DataAreaOffset8(BufferHeader *header, uint32_t offset)
{
    // Data storage area following header in buffer.
    uint8_t *dataStart = (uint8_t *)(header + 1);

    // Offset within data storage area.
    return dataStart + offset;
}

static uint32_t RoundUp(uint32_t value, uint32_t alignment)
{
    // alignment must be a power of two.
    return (value + (alignment - 1)) & ~(alignment - 1);
}

// Helper function for IntercoreRecv. Reads data from the inbound buffer,
// and wraps around to start of buffer if required. Returns updated read position.
static uint32_t ReadInboundCircular(const IntercoreComm *icc, uint32_t startPos, void *dest,
                                    size_t size)
{
    uint32_t availToEnd = icc->inboundBufSize - startPos;

    uint32_t readFromEnd = size;
    // If the available data wraps around the end of the buffer then only read
    // availToEnd bytes before subsequently reading from the start of the buffer.
    if (size > availToEnd) {
        readFromEnd = availToEnd;
    }

    uint8_t *dest8 = (uint8_t *)dest;
    const uint8_t *src8 = DataAreaOffset8(icc->inbound, startPos);
    __builtin_memcpy(dest, src8, readFromEnd);

    // If block wrapped around the end of the buffer, then read remainder from start.
    __builtin_memcpy(dest8 + readFromEnd, DataAreaOffset8(icc->inbound, 0), size - readFromEnd);

    uint32_t finalPos = startPos + size;
    if (finalPos > icc->inboundBufSize) {
        finalPos -= icc->inboundBufSize;
    }
    return finalPos;
}

IntercoreResult IntercoreRecv(IntercoreComm *icc, ComponentId *srcAppId, void *dest, size_t *size)
{
    // Don't read message content until have seen that remote write position has been updated.
    // Corresponding release occurs on high-level core.
    uint32_t remoteWritePosition;
    __atomic_load(&icc->inbound->writePosition, &remoteWritePosition, __ATOMIC_ACQUIRE);
    // Last position read from by this RTApp.
    uint32_t localReadPosition = icc->outbound->readPosition;

    // sanity check read and write positions
    INTERCORE_ASSERT(remoteWritePosition < icc->inboundBufSize);
    INTERCORE_ASSERT((remoteWritePosition % RINGBUFFER_ALIGNMENT) == 0);
    INTERCORE_ASSERT(localReadPosition < icc->inboundBufSize);
    INTERCORE_ASSERT((localReadPosition % RINGBUFFER_ALIGNMENT) == 0);

    // Get the maximum amount of available data. The actual block size may be
    // smaller than this.

    uint32_t availData;
    // If data is contiguous in buffer then difference between write and read positions...
    if (remoteWritePosition >= localReadPosition) {
        availData = remoteWritePosition - localReadPosition;
    }
    // ...else data wraps around end and resumes at start of buffer
    else {
        availData = remoteWritePosition - localReadPosition + icc->inboundBufSize;
    }

    // The amount of available data must be at least enough to hold the block size.
    // If not, caller will assume that no message was available.
    const size_t blockSizeSize = sizeof(uint32_t);
    if (availData < blockSizeSize) {
        return Intercore_Recv_NoBlockSize;
    }

    // The block size must be stored in four contiguous bytes before wraparound.
    uint32_t dataToEnd = icc->inboundBufSize - localReadPosition;
    INTERCORE_ASSERT(blockSizeSize <= dataToEnd);

    // The block size followed by the actual block can be no longer than the available data.
    uint32_t blockSize;
    localReadPosition = ReadInboundCircular(icc, localReadPosition, &blockSize, sizeof(blockSize));
    uint32_t totalBlockSize;
    // clang-tidy fails with "error: use of unknown builtin '__builtin_add_overflow_p'"
#ifndef __clang_analyzer__
    INTERCORE_ASSERT(!__builtin_add_overflow_p(blockSizeSize, blockSize, totalBlockSize));
#endif
    totalBlockSize = blockSizeSize + blockSize;
    INTERCORE_ASSERT(totalBlockSize <= availData);

    // The payload contains a sender ID (16 bytes) followed by a reserved word
    // (4 bytes) followed by the sender-supplied data.
    const uint32_t senderComponentIdSize = sizeof(*srcAppId);
    const uint32_t reservedWordSize = sizeof(uint32_t);
    const uint32_t minReqBlockSize = senderComponentIdSize + reservedWordSize;
    INTERCORE_ASSERT(blockSize >= minReqBlockSize);

    // The caller-supplied buffer must be large enough to contain the payload in the buffer,
    // excluding component ID and reserved word.
    size_t senderPayloadSize = blockSize - minReqBlockSize;
    if (senderPayloadSize > *size) {
        return Intercore_Recv_BufferTooSmall;
    }

    // Tell the caller the actual block size.
    *size = senderPayloadSize;

    // Read the sender component ID, reserved word, and app-specific payload from the block.
    // This may wraparound to the start of the buffer.
    localReadPosition = ReadInboundCircular(icc, localReadPosition, srcAppId, sizeof(*srcAppId));
    uint32_t reservedWord; // discarded
    localReadPosition =
        ReadInboundCircular(icc, localReadPosition, &reservedWord, sizeof(reservedWord));
    localReadPosition = ReadInboundCircular(icc, localReadPosition, dest, senderPayloadSize);

    // Align read position to next possible location for next buffer. This may wrap around.
    localReadPosition = RoundUp(localReadPosition, RINGBUFFER_ALIGNMENT);
    if (localReadPosition >= icc->inboundBufSize) {
        localReadPosition -= icc->inboundBufSize;
    }

    // The message content must have been retrieved before the high-level core sees the read
    // position has been updated. Corresponding acquire occurs on high-level core.
    __atomic_store(&icc->outbound->readPosition, &localReadPosition, __ATOMIC_RELEASE);

    MT3620_SignalHLCoreMessageReceived();

    return Intercore_OK;
}

// Helper function for IntercoreSend. Writes data to the outbound buffer,
// and wraps around to start of buffer if required. Returns updated write position.
static uint32_t WriteOutboundCircular(const IntercoreComm *icc, uint32_t startPos, const void *src,
                                      size_t size)
{
    uint32_t spaceToEnd = icc->outboundBufSize - startPos;

    uint32_t writeToEnd = size;
    // If the new data would wrap around the end of the buffer then only write
    // spaceToEnd bytes before subsequently writing to the start of the buffer.
    if (size > spaceToEnd) {
        writeToEnd = spaceToEnd;
    }

    const uint8_t *src8 = (const uint8_t *)src;
    uint8_t *dest8 = DataAreaOffset8(icc->outbound, startPos);
    __builtin_memcpy(dest8, src8, writeToEnd);
    // If not enough space to write all data before end of buffer, then write remainder at start.
    __builtin_memcpy(DataAreaOffset8(icc->outbound, 0), src8 + writeToEnd, size - writeToEnd);

    uint32_t finalPos = startPos + size;
    if (finalPos > icc->outboundBufSize) {
        finalPos -= icc->outboundBufSize;
    }
    return finalPos;
}

IntercoreResult IntercoreSend(IntercoreComm *icc, const ComponentId *destAppId, const void *data,
                              size_t size)
{
    if (size > INTERCORE_MAX_PAYLOAD_LEN) {
        return Intercore_Send_MessageTooLarge;
    }

    // Last position read by HLApp. Corresponding release occurs on high-level core.
    uint32_t remoteReadPosition;
    __atomic_load(&icc->inbound->readPosition, &remoteReadPosition, __ATOMIC_ACQUIRE);
    // Last position written to by RTApp.
    uint32_t localWritePosition = icc->outbound->writePosition;

    // Sanity check read and write positions.
    INTERCORE_ASSERT(remoteReadPosition < icc->outboundBufSize);
    INTERCORE_ASSERT((remoteReadPosition % RINGBUFFER_ALIGNMENT) == 0);
    INTERCORE_ASSERT(localWritePosition < icc->outboundBufSize);
    INTERCORE_ASSERT((localWritePosition % RINGBUFFER_ALIGNMENT) == 0);

    // If the read pointer is behind the write pointer, then the free space
    // wraps around, and the used space doesn't.
    uint32_t availSpace;
    if (remoteReadPosition <= localWritePosition) {
        availSpace = remoteReadPosition - localWritePosition + icc->outboundBufSize;
    } else {
        availSpace = remoteReadPosition - localWritePosition;
    }

    // Check whether there is enough space to enqueue the next block.
    uint32_t reqBlockSize = 0;
    reqBlockSize += sizeof(uint32_t);    // block size field
    reqBlockSize += sizeof(ComponentId); // destination HLApp ID
    reqBlockSize += sizeof(uint32_t);    // reserved word
    reqBlockSize += size;                // payload

    if (availSpace < reqBlockSize + RINGBUFFER_ALIGNMENT) {
        return Intercore_Send_NotEnoughBufferSpace;
    }

    // The value in the block size field does not include the space taken by the
    // block size field itself.
    uint32_t blockSizeExcSizeField = reqBlockSize - sizeof(uint32_t);
    localWritePosition = WriteOutboundCircular(icc, localWritePosition, &blockSizeExcSizeField,
                                               sizeof(blockSizeExcSizeField));
    localWritePosition =
        WriteOutboundCircular(icc, localWritePosition, destAppId, sizeof(*destAppId));
    uint32_t reservedWord = 0;
    localWritePosition =
        WriteOutboundCircular(icc, localWritePosition, &reservedWord, sizeof(reservedWord));
    localWritePosition = WriteOutboundCircular(icc, localWritePosition, data, size);

    // Advance write position to start of next possible block.
    localWritePosition = RoundUp(localWritePosition, RINGBUFFER_ALIGNMENT);
    if (localWritePosition >= icc->outboundBufSize) {
        localWritePosition -= icc->outboundBufSize;
    }

    // Ensure write position update is seen after new content has been written.
    // Corresponding acquire is on high-level core.
    __atomic_store(&icc->outbound->writePosition, &localWritePosition, __ATOMIC_RELEASE);

    MT3620_SignalHLCoreMessageSent();

    return Intercore_OK;
}
