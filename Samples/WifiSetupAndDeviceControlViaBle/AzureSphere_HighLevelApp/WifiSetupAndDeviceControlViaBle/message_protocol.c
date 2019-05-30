/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "message_protocol.h"
#include "message_protocol_private.h"
#include "message_protocol_utilities.h"
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/log.h>
#include <applibs/uart.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#define UART_RECEIVED_BUFFER_SIZE 1024u
#define UART_SEND_BUFFER_SIZE 247u // This is the max MTU size of BLE GATT.

#define REQUEST_TIMEOUT 5u

// File descriptors - initialized to invalid value.
static int epollFdRef = -1;
static int messageUartFd = -1;
static int sendRequestMessageTimerFd = -1;

// Buffer for data received via UART and index at which to write future data.
static uint8_t receiveBuffer[UART_RECEIVED_BUFFER_SIZE];
static uint16_t receiveBufferPos = 0;

// Buffer for data to be writtern via UART.
static uint8_t sendBuffer[UART_SEND_BUFFER_SIZE];

// Total amount of data in sendBuffer.
static size_t sendBufferDataLength = 0;

// Amount of data so far written to the UART.
static size_t sendBufferDataSent = 0;

// Message protocol states.
typedef enum {
    MessageProtocolState_Idle,
    MessageProtocolState_RequestOutstanding
} MessageProtocolState;

// Current state of the message protocol.
static MessageProtocolState protocolState;

// True if the EPOLLOUT event is registered for the UART fd; false if not.
static bool uartFdEpolloutEnabled = false;

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
    struct timespec disabled = {0, 0};
    SetTimerFdToPeriod(sendRequestMessageTimerFd, &disabled);

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

static void HandleReceivedMessage(EventData *eventData)
{
    // Attempt to read message from UART.
    ssize_t bytesRead = read(messageUartFd, receiveBuffer + receiveBufferPos,
                             UART_RECEIVED_BUFFER_SIZE - receiveBufferPos);
    if (bytesRead < 0) {
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

static void RequestTimeoutEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(sendRequestMessageTimerFd) != 0) {
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

static void SendUartMessage(EventData *eventData);
static EventData requestTimeoutEventData = {.eventHandler = &RequestTimeoutEventHandler};
static EventData uartReceivedEventData = {.eventHandler = &HandleReceivedMessage};
static EventData uartSendEventData = {.eventHandler = &SendUartMessage};

static void SendUartMessage(EventData *eventData)
{
    if (uartFdEpolloutEnabled) {
        RegisterEventHandlerToEpoll(epollFdRef, messageUartFd, &uartReceivedEventData, EPOLLIN);
        uartFdEpolloutEnabled = false;
    }

    while (sendBufferDataSent < sendBufferDataLength) {
        // Send as much of the remaining data as possible.
        size_t bytesLeftToSend = (size_t)(sendBufferDataLength - sendBufferDataSent);
        const uint8_t *remainingMessageToSend = sendBuffer + sendBufferDataSent;
        ssize_t bytesSent = write(messageUartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent < 0) {
            if (errno != EAGAIN) {
                Log_Debug("ERROR: Failed to write to UART: %s (%d).\n", strerror(errno), errno);
            } else {
                // Register EPOLLOUT to send the rest
                RegisterEventHandlerToEpoll(epollFdRef, messageUartFd, &uartSendEventData,
                                            EPOLLOUT);
                uartFdEpolloutEnabled = true;
            }
            return;
        }
        sendBufferDataSent += (size_t)bytesSent;
    }
}

int MessageProtocol_Init(int epollFd, int uartFd)
{
    epollFdRef = epollFd;
    messageUartFd = uartFd;

    if (RegisterEventHandlerToEpoll(epollFd, messageUartFd, &uartReceivedEventData, EPOLLIN) != 0) {
        return -1;
    }

    // Set up request timeout timer, for later use.
    struct timespec disabled = {0, 0};
    sendRequestMessageTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &disabled, &requestTimeoutEventData, EPOLLIN);
    if (sendRequestMessageTimerFd < 0) {
        return -1;
    }

    protocolState = MessageProtocolState_Idle;
    currentResponseHandler = NULL;
    eventHandlerList = NULL;
    idleHandlerList = NULL;
    return 0;
}

void MessageProtocol_Cleanup(void)
{
    CloseFdAndPrintError(sendRequestMessageTimerFd, "SendRequestMessageTimer");
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
    requestMessage->requestHeader.messageHeaderWithType.messageHeader.length = (uint16_t)(
        sizeof(MessageProtocol_RequestHeader) - sizeof(MessageProtocol_MessageHeader) + bodyLength);
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
    if (messageLength > UART_SEND_BUFFER_SIZE) {
        Log_Debug("ERROR: Request message length (%d) exceeds send buffer size.\n", messageLength);
        return;
    }
    memcpy(requestMessage->data, body, bodyLength);

    currentResponseHandler = responseHandler;
    sendBufferDataLength = messageLength;
    sendBufferDataSent = 0;

    // Start timer for response to this request.
    const struct timespec sendRequestMessageCheckPeriod = {REQUEST_TIMEOUT, 0};
    SetTimerFdToSingleExpiry(sendRequestMessageTimerFd, &sendRequestMessageCheckPeriod);
    protocolState = MessageProtocolState_RequestOutstanding;

    SendUartMessage(NULL);
}

bool MessageProtocol_IsIdle(void)
{
    return (protocolState == MessageProtocolState_Idle);
}