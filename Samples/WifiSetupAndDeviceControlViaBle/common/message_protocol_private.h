/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <sys/types.h>
#include <message_protocol_public.h>

/// <summary>The max MTU size of BLE GATT minus the request header size</summary>
#define MAX_REQUEST_DATA_SIZE 231u

/// <summary>The max MTU size of BLE GATT minus the response header size</summary>
#define MAX_RESPONSE_DATA_SIZE 231u

/// <summary>Specifies the type for a message protocol message type.</summary>
typedef uint8_t MessageProtocol_MessageType;

/// <summary>
///     Specifies the type for a message protocol message sequence number. A request/response
///     message pair must share a unique sequence number.
/// </summary>
typedef uint16_t MessageProtocol_SequenceNumber;

/// <summary>
///     The message protocol message preamble. This should always be present at the start of a
///     message.
/// </summary>
static const uint8_t MessageProtocol_MessagePreamble[] = {0x22, 0xB5, 0x58, 0xB9};

/// <summary>Message type for a request message.</summary>
static const MessageProtocol_MessageType MessageProtocol_RequestMessageType = 0x01;

/// <summary>Message type for a response message.</summary>
static const MessageProtocol_MessageType MessageProtocol_ResponseMessageType = 0x02;

/// <summary>Message type for an event message.</summary>
static const MessageProtocol_MessageType MessageProtocol_EventMessageType = 0x03;

/// <summary>
///     Data structure for a message protocol message header.
///     All messages should begin with this header. It is not intended for use directly, but instead
///     as part of a complete message structure.
/// </summary>
typedef struct {
    /// <summary>
    ///     Message preamble; must be set to <see cref="MessageProtocol_MessagePreamble" />
    /// </summary>
    uint8_t preamble[4];
    /// <summary>Length of the message, excluding this header.</summary>
    uint16_t length;
} MessageProtocol_MessageHeader;

/// <summary>
///     Data structure for a message protocol message with a specified type.
///     This is not intended for use directly, but instead as part of a complete message structure.
/// </summary>
typedef struct {
    /// <summary>The message header; see <see cref="MessageProtocol_MessageHeader" />.</summary>
    MessageProtocol_MessageHeader messageHeader;
    /// <summary>The message type; see <see cref="MessageProtocol_MessageType" />.</summary>
    MessageProtocol_MessageType type;
    /// <summary>Reserved; must be 0.</summary>
    uint8_t reserved;
} MessageProtocol_MessageHeaderWithType;

/// <summary>
///     Data structure for the body of an event message.
/// </summary>
typedef struct {
    /// <summary>Message category - see <see cref="MessageProtocol_CategoryId" />.</summary>
    MessageProtocol_CategoryId categoryId;
    /// <summary>Event identifier - see <see cref="MessageProtocol_EventId" />.</summary>
    MessageProtocol_EventId eventId;
} MessageProtocol_EventInfo;

/// <summary>
///     Data structure for a message protocol event message.
/// </summary>
typedef struct {
    /// <summary>
    ///     Message header and type; must be filled out with the correct preamble and length,
    ///     and the message type set to <see cref="MessageProtocol_EventMessageType" />.
    /// </summary>
    MessageProtocol_MessageHeaderWithType messageHeaderWithType;
    /// <summary>Details of the event. See <see cref="MessageProtocol_EventInfo" />.</summary>
    MessageProtocol_EventInfo eventInfo;
} MessageProtocol_EventMessage;

/// <summary>
///     Data structure for a message protocol request message header.
///     This forms the header for a <see cref="MessageProtocol_RequestMessage" />.
/// </summary>
typedef struct {
    /// <summary>
    ///     Message header and type; must be filled out with the correct preamble and length,
    ///     and the message type set to <see cref="MessageProtocol_RequestMessageType" />.
    /// </summary>
    MessageProtocol_MessageHeaderWithType messageHeaderWithType;
    /// <summary>Message category - see <see cref="MessageProtocol_CategoryId" />.</summary>
    MessageProtocol_CategoryId categoryId;
    /// <summary>Request identifier - see <see cref="MessageProtocol_RequestId" />.</summary>
    MessageProtocol_RequestId requestId;
    /// <summary>
    ///     Sequence number for this request - See <see cref="MessageProtocol_SequenceNumber" />.
    ///     The response message to this request must have the same sequence number.
    /// </summary>
    MessageProtocol_SequenceNumber sequenceNumber;
    /// <summary>Reserved - must all be 0.</summary>
    uint8_t reserved[2];
} MessageProtocol_RequestHeader;

/// <summary>
///     Data structure for a message protocol request message.
/// </summary>
typedef struct {
    /// <summary>The <see cref="MessageProtocol_RequestHeader" /> for this message.</summary>
    MessageProtocol_RequestHeader requestHeader;
    /// <summary>The request parameter data.</summary>
    uint8_t data[MAX_REQUEST_DATA_SIZE];
} MessageProtocol_RequestMessage;

/// <summary>
///     Data structure for a message protocol response message header.
///     This forms the header for a <see cref="MessageProtocol_ResponseMessage" />.
/// </summary>
typedef struct {
    /// <summary>
    ///     Message header and type; must be filled out with the correct preamble and length,
    ///     and the message type set to <see cref="MessageProtocol_ResponseMessageType" />.
    /// </summary>
    MessageProtocol_MessageHeaderWithType messageHeaderWithType;
    /// <summary>Message category - see <see cref="MessageProtocol_CategoryId" />.</summary>
    MessageProtocol_CategoryId categoryId;
    /// <summary>
    ///     Request identifier - see <see cref="MessageProtocol_RequestId" />. This must be the
    ///     same as the ID in the request message this is a response to.
    /// </summary>
    MessageProtocol_RequestId requestId;
    /// <summary>
    ///     Sequence number for this response - See <see cref="MessageProtocol_SequenceNumber" />.
    ///     This must be the same as the sequence number in the request message this is a response
    ///     to.
    /// </summary>
    MessageProtocol_SequenceNumber sequenceNumber;
    /// <summary>Response result - see <see cref="MessageProtocol_ResponseResult" />.</summary>
    MessageProtocol_ResponseResult responseResult;
    /// <summary>Reserved - must be 0.</summary>
    uint8_t reserved;
} MessageProtocol_ResponseHeader;

/// <summary>
///     Data structure for a message protocol response message.
/// </summary>
typedef struct {
    /// <summary>The <see cref="MessageProtocol_ResponseHeader" /> for this message.</summary>
    MessageProtocol_ResponseHeader responseHeader;
    /// <summary>The response data.</summary>
    uint8_t data[MAX_RESPONSE_DATA_SIZE];
} MessageProtocol_ResponseMessage;
