/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "blecontrol_message_protocol.h"
#include "blecontrol_message_protocol_defs.h"
#include "message_protocol.h"
#include <applibs/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define BLE_DEVICE_NAME_PREFIX "Azure_Sphere_BLE_"
static uint8_t bleDeviceName[31];
static uint8_t bleDeviceNameLength;
static BleControlMessageProtocol_AdvertisingStartedHandlerType advertisingStartedHandler = NULL;
static bool initializeDeviceRequired;

static void GenerateRandomBleDeviceName(void)
{
    bleDeviceNameLength = sizeof(BLE_DEVICE_NAME_PREFIX) + 6;
    srand((unsigned int)time(NULL));
    uint32_t randomNumber = (uint32_t)rand();
    snprintf(bleDeviceName, bleDeviceNameLength, "%s%6x", BLE_DEVICE_NAME_PREFIX, randomNumber);
}

// BLE Control responses handlers
static void InitializeBleDeviceResponseHandler(MessageProtocol_CategoryId categoryId,
                                               MessageProtocol_RequestId requestId,
                                               const uint8_t *data, size_t dataSize,
                                               MessageProtocol_ResponseResult result, bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Initialize BLE Device\" response.\n");
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Initialize BLE device\" failed with error code: %d.\n", result);
        return;
    }

    Log_Debug("INFO: \"Initialize BLE Device\" succeeded.\n");
    if (advertisingStartedHandler != NULL) {
        advertisingStartedHandler();
    }
}

static void SendInitializeBleDeviceRequest(void)
{
    BleControlMessageProtocol_InitializeBleDeviceStruct initStruct;
    memset(&initStruct, 0, sizeof(initStruct));
    memcpy(initStruct.deviceName, bleDeviceName, bleDeviceNameLength);
    initStruct.deviceNameLength = bleDeviceNameLength;
    Log_Debug("INFO: Sending \"Initialize BLE device\" request with device name set to: %s.\n",
              initStruct.deviceName);
    MessageProtocol_SendRequest(
        MessageProtocol_BleControlCategoryId, BleControlMessageProtocol_InitializeDeviceRequestId,
        (const uint8_t *)&initStruct, sizeof(initStruct), InitializeBleDeviceResponseHandler);
}

static void BleDeviceUpEventHandler(MessageProtocol_CategoryId categoryId,
                                    MessageProtocol_EventId requestId)
{
    if (MessageProtocol_IsIdle()) {
        SendInitializeBleDeviceRequest();
    } else {
        initializeDeviceRequired = true;
    }
}

static void IdleHandler(void)
{
    if (initializeDeviceRequired) {
        SendInitializeBleDeviceRequest();
    }
}

void BleControlMessageProtocol_Init(BleControlMessageProtocol_AdvertisingStartedHandlerType handler)
{
    advertisingStartedHandler = handler;

	GenerateRandomBleDeviceName();

    MessageProtocol_RegisterEventHandler(MessageProtocol_BleControlCategoryId,
                                         BleControlMessageProtocol_BleDeviceUpEventId,
                                         BleDeviceUpEventHandler);
    MessageProtocol_RegisterIdleHandler(IdleHandler);
    initializeDeviceRequired = false;
}

void BleControlMessageProtocol_Cleanup(void) {}
