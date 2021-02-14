/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"
#include "mt3620-uart-poll.h"

static const uintptr_t MAILBOX_BASE = 0x21050000;

static void ReceiveMessage(uint32_t *command, uint32_t *data);
static uint32_t GetBufferSize(uint32_t bufferBase);
static BufferHeader *GetBufferHeader(uint32_t bufferBase);
static uint8_t *DataAreaOffset8(BufferHeader *header, size_t offset);
static uint32_t *DataAreaOffset32(BufferHeader *header, size_t offset);
static uint32_t RoundUp(uint32_t value, uint32_t alignment);

static void ReceiveMessage(uint32_t *command, uint32_t *data)
{
    // FIFO_POP_CNT
    while (ReadReg32(MAILBOX_BASE, 0x58) == 0) {
        // empty.
    }

    // DATA_POP0
    *data = ReadReg32(MAILBOX_BASE, 0x54);
    // CMD_POP0
    *command = ReadReg32(MAILBOX_BASE, 0x50);
}

static uint32_t GetBufferSize(uint32_t bufferBase)
{
    return (UINT32_C(1) << (bufferBase & 0x1F));
}

static BufferHeader *GetBufferHeader(uint32_t bufferBase)
{
    return (BufferHeader *)(bufferBase & ~0x1F);
}

int GetIntercoreBuffers(BufferHeader **outbound, BufferHeader **inbound, uint32_t *bufSize)
{
    // Wait for the mailbox to be set up.
    uint32_t baseRead = 0, baseWrite = 0;
    while (true) {
        uint32_t cmd, data;
        ReceiveMessage(&cmd, &data);
        if (cmd == 0xba5e0001) {
            baseWrite = data;
        } else if (cmd == 0xba5e0002) {
            baseRead = data;
        } else if (cmd == 0xba5e0003) {
            break;
        }
    }

    uint32_t inboundBufferSize = GetBufferSize(baseRead);
    uint32_t outboundBufferSize = GetBufferSize(baseWrite);

    if (inboundBufferSize != outboundBufferSize) {
        Uart_WriteStringPoll("GetIntercoreBuffers: Mismatched buffer sizes\r\n");
        return -1;
    }

    if (inboundBufferSize <= sizeof(BufferHeader)) {
        Uart_WriteStringPoll("GetIntercoreBuffers: buffer size smaller than header\n");
        return -1;
    }

    *bufSize = inboundBufferSize - sizeof(BufferHeader);
    *inbound = GetBufferHeader(baseRead);
    *outbound = GetBufferHeader(baseWrite);

    return 0;
}

static uint8_t *DataAreaOffset8(BufferHeader *header, size_t offset)
{
    // Data storage area following header in buffer.
    uint8_t *dataStart = (uint8_t *)(header + 1);

    // Offset within data storage area.
    return dataStart + offset;
}

static uint32_t *DataAreaOffset32(BufferHeader *header, size_t offset)
{
    return (uint32_t *)DataAreaOffset8(header, offset);
}

static uint32_t RoundUp(uint32_t value, uint32_t alignment)
{
    // alignment must be a power of two.

    return (value + (alignment - 1)) & ~(alignment - 1);
}

int EnqueueData(BufferHeader *inbound, BufferHeader *outbound, uint32_t bufSize, const void *src,
                uint32_t dataSize)
{
    uint32_t remoteReadPosition = inbound->readPosition;
    uint32_t localWritePosition = outbound->writePosition;

    if (remoteReadPosition >= bufSize) {
        Uart_WriteStringPoll("EnqueueData: remoteReadPosition invalid\r\n");
        return -1;
    }

    // If the read pointer is behind the write pointer, then the free space wraps around.
    uint32_t availSpace;
    if (remoteReadPosition <= localWritePosition) {
        availSpace = remoteReadPosition - localWritePosition + bufSize;
    } else {
        availSpace = remoteReadPosition - localWritePosition;
    }

    // If there isn't enough space to enqueue a block, then abort the operation.
    if (availSpace < sizeof(uint32_t) + dataSize + RINGBUFFER_ALIGNMENT) {
        Uart_WriteStringPoll("EnqueueData: not enough space to enqueue block\r\n");
        return -1;
    }

    // Write up to end of buffer. If the block ends before then, only write up to the end of the
    // block.
    uint32_t dataToEnd = bufSize - localWritePosition;

    // There must be enough space between the write pointer and the end of the buffer to store the
    // block size as a contiguous 4-byte value. The remainder of message can wrap around.
    if (dataToEnd < sizeof(uint32_t)) {
        Uart_WriteStringPoll("EnqueueData: not enough space for block size\r\n");
        return -1;
    }

    uint32_t writeToEnd = sizeof(uint32_t) + dataSize;
    if (dataToEnd < writeToEnd) {
        writeToEnd = dataToEnd;
    }

    // Write block size to first word in block.
    *DataAreaOffset32(outbound, localWritePosition) = dataSize;
    writeToEnd -= sizeof(uint32_t);

    const uint8_t *src8 = src;
    uint8_t *dest8 = DataAreaOffset8(outbound, localWritePosition + sizeof(uint32_t));

    __builtin_memcpy(dest8, src8, writeToEnd);
    __builtin_memcpy(DataAreaOffset8(outbound, 0), src8 + writeToEnd, dataSize - writeToEnd);

    // Advance write position.
    localWritePosition =
        RoundUp(localWritePosition + sizeof(uint32_t) + dataSize, RINGBUFFER_ALIGNMENT);
    if (localWritePosition >= bufSize) {
        localWritePosition -= bufSize;
    }
    outbound->writePosition = localWritePosition;

    // SW_TX_INT_PORT[0] = 1 -> indicate message received.
    WriteReg32(MAILBOX_BASE, 0x14, 1U << 0);
    return 0;
}

int DequeueData(BufferHeader *outbound, BufferHeader *inbound, uint32_t bufSize, void *dest,
                uint32_t *dataSize)
{
    uint32_t remoteWritePosition = inbound->writePosition;
    uint32_t localReadPosition = outbound->readPosition;

	if (remoteWritePosition >= bufSize) {
        Uart_WriteStringPoll("DequeueData: remoteWritePosition invalid\r\n");
        return -1;
    }

    size_t availData;
    // If data is contiguous in buffer then difference between write and read positions...
    if (remoteWritePosition >= localReadPosition) {
        availData = remoteWritePosition - localReadPosition;
    }
    // ...else data wraps around end and resumes at start of buffer
    else {
        availData = remoteWritePosition - localReadPosition + bufSize;
    }

    // There must be at least four contiguous bytes to hold the block size.
    if (availData < sizeof(uint32_t)) {
        if (availData > 0) {
            Uart_WriteStringPoll("DequeueData: availData < 4 bytes\r\n");
        }

        return -1;
    }

    size_t dataToEnd = bufSize - localReadPosition;
    if (dataToEnd < sizeof(uint32_t)) {
        Uart_WriteStringPoll("DequeueData: dataToEnd < 4 bytes\r\n");
        return -1;
    }

    uint32_t blockSize = *DataAreaOffset32(inbound, localReadPosition);

    // Ensure the block size is no greater than the available data.
    if (blockSize + sizeof(uint32_t) > availData) {
        Uart_WriteStringPoll("DequeueData: message size greater than available data\r\n");
        return -1;
    }

    // Abort if the caller-supplied buffer is not large enough to hold the message.
    if (blockSize > *dataSize) {
        Uart_WriteStringPoll("DequeueData: message too large for buffer\r\n");
        *dataSize = blockSize;
        return -1;
    }

    // Tell the caller the actual block size.
    *dataSize = blockSize;

    // Read up to the end of the buffer. If the block ends before then, only read up to the end
    // of the block.
    uint32_t readFromEnd = dataToEnd - sizeof(uint32_t);
    if (blockSize < readFromEnd) {
        readFromEnd = blockSize;
    }

    const uint8_t *src8 = DataAreaOffset8(inbound, localReadPosition + sizeof(uint32_t));
    uint8_t *dest8 = dest;
    __builtin_memcpy(dest8, src8, readFromEnd);
    // If block wrapped around the end of the buffer, then read remainder from start.
    __builtin_memcpy(dest8 + readFromEnd, DataAreaOffset8(inbound, 0), blockSize - readFromEnd);

    // Round read position to next aligned block, and wraparound end of buffer if required.
    localReadPosition =
        RoundUp(localReadPosition + sizeof(uint32_t) + blockSize, RINGBUFFER_ALIGNMENT);
    if (localReadPosition >= bufSize) {
        localReadPosition -= bufSize;
    }

    outbound->readPosition = localReadPosition;

    // SW_TX_INT_PORT[1] = 1 -> indicate message received.
    WriteReg32(MAILBOX_BASE, 0x14, 1U << 1);

    return 0;
}
