/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "mt3620-baremetal.h" // for Callback

/// <summary>
///     When sending a message, this is the recipient HLApp's component ID.
///     When receiving a message, this is the sender HLApp's component ID.
/// </summary>
typedef struct {
    /// <summary>4-byte little-endian word</summary>
    uint32_t data1;
    /// <summary>2-byte little-endian half</summary>
    uint16_t data2;
    /// <summary>2-byte little-endian half</summary>
    uint16_t data3;
    /// <summary>2 bytes (big-endian) followed by 6 bytes (big-endian)</summary>
    uint8_t data4[8];
} ComponentId;

/// <summary>Blocks inside the shared buffer have this alignment.</summary>
#define RINGBUFFER_ALIGNMENT 16
/// <summary>
///     Maximum payload size in bytes. This does not include a header which
///     is prepended by <see cref="IntercoreSend" />.
/// </summary>
#define INTERCORE_MAX_PAYLOAD_LEN 1040

typedef struct BufferHeaderImpl BufferHeader;

/// <summary>
///     Encapsulates information which is used to send data to, and receive data from HLApps.
///     This object is a handle, so the caller should not read or write the contained data.
///     Initialize this object with <see cref="SetupIntercoreComm" />.
/// </summary>
typedef struct {
    /// <summary>Buffer used to send data from the HLApp to the RTApp.</summary>
    BufferHeader *inbound;
    /// <summary>Buffer used to send data from the RTApp to the HLApp.</summary>
    BufferHeader *outbound;
    /// <summary>Inbound buffer size in bytes.</summary>
    uint32_t inboundBufSize;
    /// <summary>Outbound buffer size in bytes.</summary>
    uint32_t outboundBufSize;
} IntercoreComm;

/// <summary>
///     Error codes which can occur when using the intercore buffers.
///     These are errors which can occur during normal use, for example
///     no message available, or not enough space in buffer. The logical
///     layer asserts for unexpected conditions such as a corrupt buffer.
/// </summary>
typedef enum {
    /// <summary>Operation completed successfully.</summary>
    Intercore_OK = 0,

    /// <summary>
    ///     The incoming buffer did not contain enough space for the next
    ///     block size. In practical terms, this usually means the incoming buffer
    ///     is empty, so there is no message to retrieve.
    /// </summary>
    Intercore_Recv_NoBlockSize = 0x10,

    /// <summary>The supplied buffer size was too small to hold the incoming message.</summary>
    Intercore_Recv_BufferTooSmall = 0x11,

    /// <summary>
    ///     The supplied message was too large. Even if there is enough space in
    ///     the buffer, messages are limited to a 1040-byte payload.
    /// </summary>
    Intercore_Send_MessageTooLarge = 0x20,

    /// <summary>There was not enough space in the buffer to send the supplied message.</summary>
    Intercore_Send_NotEnoughBufferSpace = 0x21
} IntercoreResult;

/// <summary>
///     Populates the supplied IntercoreComm object by getting buffer information
///     from the high-level core.
/// </summary>
/// <param name="icc">Handle object to populate.</param>
/// <param name="recvCallback">
///     Function which is called in DPC context when an incoming message arrives.
///     The application must call <see cref="InvokeDeferredProcs" /> to run the function.
/// </param>
/// <returns>Intercore_OK on success, another IntercoreResult value otherwise.</returns>
IntercoreResult SetupIntercoreComm(IntercoreComm *icc, Callback recvCallback);

/// <summary>
///     Retrieves the next incoming message from the HLApp.
/// </summary>
/// <param name="icc">Handle which was initialized by <see cref="SetupIntercoreComm" /></param>
/// <param name="sender">Component ID which will be populated with the sending HLApp's ID.</param>
/// <param name="dest">Buffer which will store the payload sent by the HLApp.</param>
/// <param name="size">
///     On entry, contains the size of the destination buffer. On exit, set to
///     the amount of payload data.
/// </param>
/// <returns>
///     <see cref="Intercore_OK" /> if the message was retrieved successfully;
///     <see cref="Intercore_Recv_NoBlockSize" /> if there was no message to retrieve; or
///     <see cref="Intercore_Recv_BufferTooSmall" /> if the supplied buffer was not large
///     enough to contain the message payload. In the last case, the input parameters are
///     not modified, and the message is left in the buffer.
/// </returns>
IntercoreResult IntercoreRecv(IntercoreComm *icc, ComponentId *sender, void *dest, size_t *size);

/// <summary>Sends a message to the HLApp.</summary>
/// <param name="icc">Handle which was initialized by <see cref="SetupIntercoreComm" /></param>
/// <param name="recipient">HLApp which should receive the message.</param>
/// <param name="data">Data to send to the HLApp.</param>
/// <param name="size">Amount of data in bytes.</param>
/// <returns>
///     <see cref="Intercore_OK" /> if the message was successfully placed
///     into the outbound buffer; <see cref="Intercore_Send_MessageTooLarge"> if the message
///     was greater than 1040 bytes, in which case nothing is sent; or
///     <see cref="Intercore_Send_NotEnoughBufferSpace" /> if there was not enough space
///     in the buffer to send the message.
/// </returns>
IntercoreResult IntercoreSend(IntercoreComm *icc, const ComponentId *recipient, const void *data,
                              size_t size);
