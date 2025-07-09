/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>
#include "main.h"

#include "messages.h"
#include "message_protocol_utilities.h"

static void ReadMessageNextByteAsync(void);

static void HandleRequest(const MessageProtocol_RequestMessage *request);
static void HandleInitRequest(const MessageProtocol_RequestMessage *request);
static void HandleTelemetryRequest(const MessageProtocol_RequestMessage *request);
static void HandleSetLedRequest(const MessageProtocol_RequestMessage *request);

static void SendMessageLen(uint8_t *msg, uint16_t len);
static void SendResponse(
	const MessageProtocol_RequestMessage *request, void *body, size_t bodyLength);

static __IO ITStatus rxStatus;
static size_t rxBytesReceived;
static uint8_t _Alignas(MessageProtocol_RequestMessage) rxBuffer[sizeof(MessageProtocol_RequestMessage)];

// Moved from RESET -> SET when TX completes.
static __IO ITStatus txStatus;
static MessageProtocol_ResponseMessage txResponse;

void ReadMessageAsync(void)
{
	rxStatus = RESET;
	rxBytesReceived = 0;

	ReadMessageNextByteAsync();
}

// Read a single byte from the UART which is connected to the Azure Sphere device.
static void ReadMessageNextByteAsync(void)
{
	if (HAL_UART_Receive_IT(&huart2, &rxBuffer[rxBytesReceived], 1) != HAL_OK) {
		Error_Handler();
	}
}

// Called when the MCU has received a single byte from the Azure Sphere device.
void HAL_UART_RxCpltCallback(UART_HandleTypeDef * handle)
{
	// If latest data would overflow RX buffer then abort.
	size_t currLength = ++rxBytesReceived;
	if (currLength > sizeof(rxBuffer)) {
		Error_Handler();
	}

	// If still in header and does not match expected header then discard.
	// This discards noise at the beginning of the transfer.
	if (currLength <= sizeof(MessageProtocol_MessagePreamble)) {
		if (rxBuffer[currLength - 1] != MessageProtocol_MessagePreamble[currLength - 1]) {
			rxBytesReceived = 0;
			ReadMessageNextByteAsync();
			return;
		}
	}

	// If received an entire message, set a flag to handle the message when
	// the ISR completes. Otherwise, read another byte from the Azure Sphere device.
	if (MessageProtocol_IsMessageComplete(rxBuffer, currLength)) {
		rxStatus = SET;
	} else {
		ReadMessageNextByteAsync();
	}
}

// Called from non-interrupt context to handle a message.
void HandleMessage(void)
{
	// Do nothing if a completed message has not yet been received.
	if (rxStatus == RESET) {
		return;
	}

	// Listen for the next command. This should occur before the response
	// has been sent because the attached device may send the next request
	// before the MCU has begun to wait for it.
	ReadMessageAsync();

	const MessageProtocol_MessageHeaderWithType *header = (MessageProtocol_MessageHeaderWithType *) rxBuffer;
	if (header->type == MessageProtocol_RequestMessageType) {
		HandleRequest((const MessageProtocol_RequestMessage *) header);
	}

	// Abort if unrecognized message type.
	else {
		Error_Handler();
	}
}

static void HandleRequest(const MessageProtocol_RequestMessage *request)
{
    if (request->requestHeader.requestId == MessageProtocol_McuToCloud_Init) {
    	HandleInitRequest(request);
    } else if (request->requestHeader.requestId == MessageProtocol_McuToCloud_RequestTelemetry) {
    	HandleTelemetryRequest(request);
    } else if (request->requestHeader.requestId == MessageProtocol_McuToCloud_SetLed) {
    	HandleSetLedRequest(request);
    }

    // Abort if unrecognized request type.
    else {
    	Error_Handler();
    }
}

static void HandleInitRequest(const MessageProtocol_RequestMessage *request)
{
	MessageProtocol_McuToCloud_InitStruct i = {
		.protocolVersion = MessageProtocol_McuToCloud_ProtocolVersion
	};

	SendResponse(request, &i, sizeof(i));
}

static void HandleTelemetryRequest(const MessageProtocol_RequestMessage *request)
{
	float batteryLevel = ReadBatteryLevel();

	MessageProtocol_McuToCloud_TelemetryStruct t = {
		.lifetimeTotalDispenses = state.issuedDispenses,
		.lifetimeTotalStockedDispenses = state.stockedDispenses,
		.capacity = state.machineCapacity,
		.batteryLevel = batteryLevel
	};

	SendResponse(request, &t, sizeof(t));
}

static void HandleSetLedRequest(const MessageProtocol_RequestMessage *request)
{
	const MessageProtocol_McuToCloud_SetLedStruct *sls =
		(const MessageProtocol_McuToCloud_SetLedStruct *) request->data;

	SetFlavor(sls->red != 0, sls->green != 0, sls->blue != 0);

	// Echo back LEDStruct as a response.
	SendResponse(request, (void *)sls, sizeof(*sls));
}

static void SendMessageLen(uint8_t *msg, uint16_t len)
{
	txStatus = RESET;

	if (HAL_UART_Transmit_IT(&huart2, msg, len) != HAL_OK) {
		Error_Handler();
	}

	while (txStatus != SET) {
		// empty.
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
	txStatus = SET;
}

// Populates the response object based on the supplied request and copies
// the supplied payload (if any) into the response.
static void SendResponse(
	const MessageProtocol_RequestMessage *request, void *body, size_t bodyLength)
{
	memset(&txResponse, 0, sizeof(txResponse));

    memcpy(&txResponse.responseHeader.messageHeaderWithType.messageHeader.preamble,
        MessageProtocol_MessagePreamble, sizeof(MessageProtocol_MessagePreamble));
	txResponse.responseHeader.messageHeaderWithType.messageHeader.length =
		(uint16_t) (
			sizeof(MessageProtocol_ResponseHeader) - sizeof(MessageProtocol_MessageHeader) + bodyLength);
	txResponse.responseHeader.messageHeaderWithType.type = MessageProtocol_ResponseMessageType;
	txResponse.responseHeader.messageHeaderWithType.reserved = 0;

	txResponse.responseHeader.categoryId = request->requestHeader.categoryId;
	txResponse.responseHeader.requestId = request->requestHeader.requestId;
	txResponse.responseHeader.sequenceNumber = request->requestHeader.sequenceNumber;
	txResponse.responseHeader.responseResult = 0;
	txResponse.responseHeader.reserved = 0;

	if (body != NULL) {
		memcpy(&txResponse.data, body, bodyLength);
	}

	SendMessageLen((uint8_t *)&txResponse, sizeof(txResponse));
}

