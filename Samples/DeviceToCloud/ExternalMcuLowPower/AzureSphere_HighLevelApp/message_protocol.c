/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <applibs/log.h>
#include <applibs/eventloop.h>

#include "message_protocol.h"
#include "message_protocol_private.h"
#include "message_protocol_utilities.h"
#include "applibs_versions.h"
#include "eventloop_timer_utilities.h"
#include "exitcodes.h"

#define REQUEST_TIMEOUT 5u

#define RECEIVED_BUFFER_SIZE 1024u
#define SEND_BUFFER_SIZE 1024u

static EventLoop *eventLoopRef = NULL;

static EventLoopTimer *requestTimeoutTimer = NULL;

static Transport_ReadFunctionType transportReadFunction = NULL;
static Transport_WriteFunctionType transportWriteFunction = NULL;

// Buffer for data received via transport and index at which to write future data.
static uint8_t receiveBuffer[RECEIVED_BUFFER_SIZE];
static uint16_t receiveBufferPos = 0;

// Buffer in which to assemble messages
static uint8_t sendBuffer[SEND_BUFFER_SIZE];

// Message protocol states.
typedef enum {
    MessageProtocolState_Idle,
    MessageProtocolState_RequestOutstanding
} MessageProtocolState;

// Current state of the message protocol.
static MessageProtocolState protocolState;

static MessageProtocol_ResponseHandlerType currentResponseHandler;

// Request sequence number
static uint16_t currentSequenceNumber = 0;

// Event handlers list
struct EventHandlerNode {
    MessageProtocol_CategoryId categoryId;
    MessageProtocol_EventId eventId;
    MessageProtocol_EventHandlerType handler;
    struct EventHandlerNode *nextNode;
};
static struct EventHandlerNode *eventHandlerList;

// Idle handlers list
struct IdleHandlerNode {
    MessageProtocol_IdleHandlerType handler;
    struct IdleHandlerNode *nextNode;
};
static struct IdleHandlerNode *idleHandlerList;

static void RemoveFirstCompleteMessage(void)
{
    MessageProtocol_MessageHeader *messageHeader = (MessageProtocol_MessageHeader *)receiveBuffer;
    uint16_t topMessageLength =
        (uint16_t)(messageHeader->length + sizeof(MessageProtocol_MessageHeader));
    if (receiveBufferPos == topMessageLength) {
        // There is only one complete message in the buffer: set receivedBufferIndex to 0.
        receiveBufferPos = 0;
    } else if (receiveBufferPos > topMessageLength) {
        // There is more than one complete message in the buffer: remove the top message, move
        // the rest forward and subtract the message length from receiveBufferPos.
        receiveBufferPos = (uint16_t)(receiveBufferPos - topMessageLength);
        memmove(receiveBuffer, receiveBuffer + topMessageLength, receiveBufferPos);
    }
}

static void RemoveInvalidBytesBeforePreamble(void)
{
    size_t preambleSize = sizeof(MessageProtocol_MessagePreamble);
    bool foundPreamble = false;
    size_t pos = 0;

    while (pos < receiveBufferPos) {
        size_t remainingDataSize = receiveBufferPos - pos;
        // Check whether we can find a complete or partial preamble by the end of current data.
        size_t checkPreambleSize =
            (remainingDataSize >= preambleSize) ? preambleSize : remainingDataSize;
        if (memcmp(MessageProtocol_MessagePreamble, receiveBuffer + pos, checkPreambleSize) == 0) {
            foundPreamble = true;
            break;
        }
        ++pos;
    }
    uint16_t validMessageLength = (uint16_t)(receiveBufferPos - pos);
    receiveBufferPos = validMessageLength;

    // Found complete or partial preamble. If it's not at the beginning of the
    // message, move everything between i and (pos - 1) to the front, otherwise
    // doing nothing as receivedBufferIndex is set to 0 already.
    if (pos > 0 && foundPreamble) {
        memmove(receiveBuffer, receiveBuffer + pos, validMessageLength);
    }
}

static MessageProtocol_EventInfo *GetEventInfo(uint8_t *message, uint16_t messageLength)
{
    MessageProtocol_MessageHeader *messageHeader = (MessageProtocol_MessageHeader *)message;
    if (messageLength <
            sizeof(MessageProtocol_MessageHeaderWithType) + sizeof(MessageProtocol_EventInfo) ||
        messageHeader->length + sizeof(MessageProtocol_MessageHeader) !=
            sizeof(MessageProtocol_MessageHeaderWithType) + sizeof(MessageProtocol_EventInfo)) {
        Log_Debug("ERROR: Received invalid event message - incorrect length.\n");
        return NULL;
    }
    MessageProtocol_EventMessage *eventMessage = (MessageProtocol_EventMessage *)(message);
    return &(eventMessage->eventInfo);
}

static void CallIdleHandlers(void)
{
    // Call all registered idle handlers as long as protocol state is still idle.
    struct IdleHandlerNode *current = idleHandlerList;
    while (current != NULL && protocolState == MessageProtocolState_Idle) {
        current->handler();
        current = current->nextNode;
    }
}

static void CallEventHandler(void)
{
    MessageProtocol_EventInfo *eventInfo = GetEventInfo(receiveBuffer, receiveBufferPos);
    if (eventInfo == NULL) {
        Log_Debug("ERROR: Received malformed event message.\n");
        return;
    }

    struct EventHandlerNode *current = eventHandlerList;
    while (current != NULL) {
        if (current->categoryId == eventInfo->categoryId &&
            current->eventId == eventInfo->eventId && current->handler != NULL) {
            current->handler(current->categoryId, current->eventId);
            return;
        }
        current = current->nextNode;
    }
    Log_Debug("ERROR: Received event message with unknown Category ID and Event ID: 0x%x, 0x%x.\n",
              eventInfo->categoryId, eventInfo->eventId);
}

static void CallResponseHandler(void)
{
    MessageProtocol_ResponseMessage *responseMessage =
        (MessageProtocol_ResponseMessage *)(receiveBuffer);

    if (receiveBufferPos < sizeof(MessageProtocol_ResponseHeader) ||
        responseMessage->responseHeader.messageHeaderWithType.messageHeader.length +
                sizeof(MessageProtocol_MessageHeader) <
            sizeof(MessageProtocol_ResponseHeader)) {
        Log_Debug("ERROR: Received invalid response message - too short.\n");
        return;
    }

    if (currentSequenceNumber != responseMessage->responseHeader.sequenceNumber) {
        Log_Debug("ERROR: Received a response with invalid sequence number: %x.\n",
                  responseMessage->responseHeader.sequenceNumber);
        return;
    }

    if (protocolState != MessageProtocolState_RequestOutstanding) {
        Log_Debug("ERROR: Received a response when not expecting one\n");
        return;
    }
    protocolState = MessageProtocolState_Idle;
    DisarmEventLoopTimer(requestTimeoutTimer);

    MessageProtocol_ResponseHandlerType handler = currentResponseHandler;
    currentResponseHandler = NULL;

    if (handler != NULL) {
        size_t dataLength =
            responseMessage->responseHeader.messageHeaderWithType.messageHeader.length +
            sizeof(MessageProtocol_MessageHeader) - sizeof(MessageProtocol_RequestHeader);
        handler(responseMessage->responseHeader.categoryId,
                responseMessage->responseHeader.requestId, responseMessage->data, dataLength,
                responseMessage->responseHeader.responseResult, false);
    }

    CallIdleHandlers();
}

void MessageProtocol_HandleReceivedMessage(void)
{
    // Attempt to read message from UART.
    ssize_t bytesRead = transportReadFunction(receiveBuffer + receiveBufferPos,
                                              RECEIVED_BUFFER_SIZE - receiveBufferPos);
    if (bytesRead == -1) {
        Log_Debug("ERROR: Could not read from UART: %s (%d).\n", strerror(errno), errno);
        return;
    }

    if (bytesRead > 0) {
        receiveBufferPos = (uint16_t)(receiveBufferPos + bytesRead);
        // Messages in the receive buffer should always start with a preamble, so remove all invalid
        // bytes before the preamble.
        RemoveInvalidBytesBeforePreamble();
        while (MessageProtocol_IsMessageComplete(receiveBuffer, (uint8_t)receiveBufferPos)) {
            // We received a complete message, call its handler.
            MessageProtocol_MessageHeaderWithType *messageHeader =
                (MessageProtocol_MessageHeaderWithType *)receiveBuffer;
            if (messageHeader->type == MessageProtocol_EventMessageType) {
                CallEventHandler();
            } else if (messageHeader->type == MessageProtocol_ResponseMessageType) {
                CallResponseHandler();
            } else {
                Log_Debug("ERROR: Skipping message: unknown or invalid message type.\n");
            }
            // We have finished with this message now, so remove it from the receive buffer.
            RemoveFirstCompleteMessage();
        }
    }
}

static void RequestTimeoutEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(requestTimeoutTimer) != 0) {
        return;
    }

    // Timed out waiting for response message: change back to Idle state and call the response
    // handler to inform it that the request has timed out.
    protocolState = MessageProtocolState_Idle;
    MessageProtocol_ResponseHandlerType handler = currentResponseHandler;
    currentResponseHandler = NULL;
    if (handler != NULL) {
        MessageProtocol_RequestMessage *requestMessage =
            (MessageProtocol_RequestMessage *)sendBuffer;
        handler(requestMessage->requestHeader.categoryId, requestMessage->requestHeader.requestId,
                NULL, 0, 0, true);
    }

    // We are idle now, so call the idle handlers.
    CallIdleHandlers();
}

ExitCode MessageProtocol_Initialize(EventLoop *el, Transport_ReadFunctionType readFunction,
                                    Transport_WriteFunctionType writeFunction)
{
    eventLoopRef = el;
    transportReadFunction = readFunction;
    transportWriteFunction = writeFunction;

    requestTimeoutTimer = CreateEventLoopDisarmedTimer(eventLoopRef, RequestTimeoutEventHandler);

    if (requestTimeoutTimer == NULL) {
        return ExitCode_MsgProtoInit_Timer;
    }

    protocolState = MessageProtocolState_Idle;
    currentResponseHandler = NULL;
    eventHandlerList = NULL;
    idleHandlerList = NULL;
    return ExitCode_Success;
}

void MessageProtocol_Cleanup(void)
{
    DisposeEventLoopTimer(requestTimeoutTimer);
    eventLoopRef = NULL;
    transportReadFunction = NULL;
    transportWriteFunction = NULL;

    // Free all event handlers in the list.
    struct EventHandlerNode *currentEventHandler = NULL;
    while (eventHandlerList != NULL) {
        currentEventHandler = eventHandlerList;
        eventHandlerList = eventHandlerList->nextNode;
        free(currentEventHandler);
    }
    // Free all idle handlers in the list.
    struct IdleHandlerNode *currentIdleHandler = NULL;
    while (idleHandlerList != NULL) {
        currentIdleHandler = idleHandlerList;
        idleHandlerList = idleHandlerList->nextNode;
        free(currentIdleHandler);
    }
}

void MessageProtocol_RegisterEventHandler(MessageProtocol_CategoryId categoryId,
                                          MessageProtocol_EventId eventId,
                                          MessageProtocol_EventHandlerType handler)
{
    struct EventHandlerNode *node = malloc(sizeof(struct EventHandlerNode));
    memset(node, 0, sizeof(struct EventHandlerNode));
    node->categoryId = categoryId;
    node->eventId = eventId;
    node->handler = handler;
    // Add it to the head of linked list.
    node->nextNode = eventHandlerList;
    eventHandlerList = node;
}

void MessageProtocol_RegisterIdleHandler(MessageProtocol_IdleHandlerType handler)
{
    struct IdleHandlerNode *node = malloc(sizeof(struct IdleHandlerNode));
    memset(node, 0, sizeof(struct IdleHandlerNode));
    node->handler = handler;
    // Add it to the head of linked list.
    node->nextNode = idleHandlerList;
    idleHandlerList = node;
}

void MessageProtocol_SendRequest(MessageProtocol_CategoryId categoryId,
                                 MessageProtocol_RequestId requestId, const uint8_t *body,
                                 size_t bodyLength,
                                 MessageProtocol_ResponseHandlerType responseHandler)
{
    if (protocolState != MessageProtocolState_Idle) {
        Log_Debug("INFO: Protocol busy, can't send request: %x, %x.\n", categoryId, requestId);
        return;
    }

    // Set request message data in-place.
    MessageProtocol_RequestMessage *requestMessage = (MessageProtocol_RequestMessage *)sendBuffer;
    memcpy(requestMessage->requestHeader.messageHeaderWithType.messageHeader.preamble,
           MessageProtocol_MessagePreamble, sizeof(MessageProtocol_MessagePreamble));
    requestMessage->requestHeader.messageHeaderWithType.messageHeader.length =
        (uint16_t)(sizeof(MessageProtocol_RequestHeader) - sizeof(MessageProtocol_MessageHeader) +
                   bodyLength);
    requestMessage->requestHeader.messageHeaderWithType.type = MessageProtocol_RequestMessageType;
    requestMessage->requestHeader.messageHeaderWithType.reserved = 0x00;
    requestMessage->requestHeader.categoryId = categoryId;
    requestMessage->requestHeader.requestId = requestId;
    requestMessage->requestHeader.sequenceNumber = ++currentSequenceNumber;
    memset(requestMessage->requestHeader.reserved, 0, 2);

    // Check message length is within UART send buffer size limit.
    uint16_t messageLength =
        (uint16_t)(requestMessage->requestHeader.messageHeaderWithType.messageHeader.length +
                   sizeof(MessageProtocol_MessageHeader));
    if (messageLength > SEND_BUFFER_SIZE) {
        Log_Debug("ERROR: Request message length (%d) exceeds send buffer size.\n", messageLength);
        return;
    }
    memcpy(requestMessage->data, body, bodyLength);

    currentResponseHandler = responseHandler;

    // Start timer for response to this request.
    const struct timespec sendRequestMessageCheckPeriod = {REQUEST_TIMEOUT, 0};
    SetEventLoopTimerOneShot(requestTimeoutTimer, &sendRequestMessageCheckPeriod);
    protocolState = MessageProtocolState_RequestOutstanding;

    transportWriteFunction(sendBuffer, messageLength);
}

bool MessageProtocol_IsIdle(void)
{
    return (protocolState == MessageProtocolState_Idle);
}