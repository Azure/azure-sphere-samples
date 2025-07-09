/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "blecontrol_message_protocol.h"
#include "blecontrol_message_protocol_defs.h"
#include "message_protocol.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BLE_DEVICE_NAME_PREFIX "Azure_Sphere_BLE_"
#define BLE_DEVICE_NAME_MAX_LEN 31
static uint8_t bleDeviceName[BLE_DEVICE_NAME_MAX_LEN];
static uint8_t bleDeviceNameLength;
#define BLE_PASSKEY_LEN 6
static uint8_t blePasskey[BLE_PASSKEY_LEN + 1];
static BleControlMessageProtocol_StateChangeHandlerType bleStateChangeHandler = NULL;
static bool initializeDeviceRequired;
static bool setPasskeyRequired;
static bool changeBleAdvertisingModeRequired;
static bool deleteAllBleBondsDeviceRequired;
static int bleAdvertiseToAllTimerFd = -1;

static BleControlMessageProtocol_BleAdvertisingMode currentAdvertisingMode;
static BleControlMessageProtocol_BleAdvertisingMode desiredAdvertisingMode;
static BleControlMessageProtocolState blePublicState;

static void GenerateRandomBleDeviceName(void)
{
    bleDeviceNameLength = sizeof(BLE_DEVICE_NAME_PREFIX) + 6;
    srand((unsigned int)time(NULL));
    uint32_t randomNumber = (uint32_t)rand();
    snprintf(bleDeviceName, bleDeviceNameLength, "%s%6x", BLE_DEVICE_NAME_PREFIX, randomNumber);
}

static void GenerateRandomBlePasskey(void)
{
    srand((unsigned int)time(NULL));
    // BLE Passkey is a fixed-size six digit number (from "000000" to "999999").
    for (size_t i = 0; i < BLE_PASSKEY_LEN; ++i) {
        snprintf(blePasskey + i, 2, "%d", rand() % 10);
    }
}

static void ChangeBleProtocolState(BleControlMessageProtocolState state);

static void SetPasskeyResponseHandler(MessageProtocol_CategoryId categoryId,
                                      MessageProtocol_RequestId requestId, const uint8_t *data,
                                      size_t dataSize, MessageProtocol_ResponseResult result,
                                      bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Set Passkey\" response.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Set Passkey\" failed with error code: %u.\n", result);
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    Log_Debug("INFO: \"Set Passkey\" succeeded.\n");

    // If we were in the initialization stage, this response means that initialization has now
    // completed and nRF52 is in default advertising mode - to bonded devices.
    if (blePublicState == BleControlMessageProtocolState_Uninitialized) {
        currentAdvertisingMode = BleControlMessageProtocol_AdvertisingToBondedDevicesMode;
        ChangeBleProtocolState(BleControlMessageProtocolState_AdvertiseToBondedDevices);
    }
}

static void SendSetPasskeyRequest(void)
{
    if (MessageProtocol_IsIdle()) {
        BleControlMessageProtocol_SetPasskeyStruct passkey;
        memset(&passkey, 0, sizeof(passkey));
        GenerateRandomBlePasskey();
        memcpy(passkey.passkey, blePasskey, BLE_PASSKEY_LEN);

        Log_Debug("INFO: Sending \"Set Passkey\" request.\n");
        MessageProtocol_SendRequest(
            MessageProtocol_BleControlCategoryId, BleControlMessageProtocol_SetPasskeyRequestId,
            (const uint8_t *)&passkey, sizeof(passkey), &SetPasskeyResponseHandler);
        setPasskeyRequired = false;
    } else {
        setPasskeyRequired = true;
    }
}

static void InitializeBleDeviceResponseHandler(MessageProtocol_CategoryId categoryId,
                                               MessageProtocol_RequestId requestId,
                                               const uint8_t *data, size_t dataSize,
                                               MessageProtocol_ResponseResult result, bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Initialize BLE Device\" response.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Initialize BLE Device\" failed with error code: %u.\n", result);
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (blePublicState == BleControlMessageProtocolState_Uninitialized) {
        // Do the next initialization step - send passkey.
        Log_Debug("INFO: \"Initialize BLE Device\" succeeded.\n");
        SendSetPasskeyRequest();
    } else {
        // This response should only be received during the initialization phase.
        Log_Debug("ERROR: \"Initialize BLE Device\" response received when not expected.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
    }
}

static void ChangeBleAdvertisingModeResponseHandler(MessageProtocol_CategoryId categoryId,
                                                    MessageProtocol_RequestId requestId,
                                                    const uint8_t *data, size_t dataSize,
                                                    MessageProtocol_ResponseResult result,
                                                    bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Change BLE Mode\" response.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Change BLE Mode\" failed with error code: %u.\n", result);
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (dataSize != sizeof(BleControlMessageProtocol_ChangeBleAdvertisingModeStruct)) {
        Log_Debug("ERROR: \"Change BLE Mode\" response is invalid.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    BleControlMessageProtocol_ChangeBleAdvertisingModeStruct *newBleAdvertisingModeData =
        (BleControlMessageProtocol_ChangeBleAdvertisingModeStruct *)data;
    BleControlMessageProtocolState newState;
    currentAdvertisingMode = newBleAdvertisingModeData->mode;
    switch (newBleAdvertisingModeData->mode) {
    case BleControlMessageProtocol_AdvertisingToBondedDevicesMode:
        newState = BleControlMessageProtocolState_AdvertiseToBondedDevices;
        break;
    case BleControlMessageProtocol_AdvertisingToAllMode:
        newState = BleControlMessageProtocolState_AdvertisingToAllDevices;
        break;
    default:
        newState = BleControlMessageProtocolState_Error;
        Log_Debug("ERROR: \"Change BLE Mode\" response has an invalid mode.\n");
        break;
    }
    ChangeBleProtocolState(newState);
}

static void SendDeleteAllBondsResponseHandler(MessageProtocol_CategoryId categoryId,
                                              MessageProtocol_RequestId requestId,
                                              const uint8_t *data, size_t dataSize,
                                              MessageProtocol_ResponseResult result, bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Delete all BLE bonds\" response.\n");
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Delete all BLE bonds\" failed with error code: %u.\n", result);
        ChangeBleProtocolState(BleControlMessageProtocolState_Error);
        return;
    }
}

static void SendInitializeBleDeviceRequest(void)
{
    if (MessageProtocol_IsIdle()) {
        BleControlMessageProtocol_InitializeBleDeviceStruct initStruct;
        memset(&initStruct, 0, sizeof(initStruct));
        memcpy(initStruct.deviceName, bleDeviceName, bleDeviceNameLength);
        initStruct.deviceNameLength = bleDeviceNameLength;
        Log_Debug("INFO: Sending \"Initialize BLE device\" request with device name set to: %s.\n",
                  initStruct.deviceName);
        MessageProtocol_SendRequest(MessageProtocol_BleControlCategoryId,
                                    BleControlMessageProtocol_InitializeDeviceRequestId,
                                    (const uint8_t *)&initStruct, sizeof(initStruct),
                                    InitializeBleDeviceResponseHandler);
        initializeDeviceRequired = false;
    } else {
        initializeDeviceRequired = true;
    }
}

static void SendChangeBleAdvertisingModeRequest(
    BleControlMessageProtocol_BleAdvertisingMode newMode)
{
    if (currentAdvertisingMode != newMode) {
        if (MessageProtocol_IsIdle()) {
            BleControlMessageProtocol_ChangeBleAdvertisingModeStruct bleAdvertisingMode;
            memset(&bleAdvertisingMode, 0, sizeof(bleAdvertisingMode));
            bleAdvertisingMode.mode = newMode;
            Log_Debug("INFO: Sending \"Change BLE mode\" request mode set to: %d.\n", newMode);
            MessageProtocol_SendRequest(MessageProtocol_BleControlCategoryId,
                                        BleControlMessageProtocol_ChangeBleAdvertisingModeRequestId,
                                        (const uint8_t *)&bleAdvertisingMode,
                                        sizeof(bleAdvertisingMode),
                                        ChangeBleAdvertisingModeResponseHandler);
            changeBleAdvertisingModeRequired = false;
        } else {
            desiredAdvertisingMode = newMode;
            changeBleAdvertisingModeRequired = true;
        }
    } else {
        changeBleAdvertisingModeRequired = false;
    }
}

static void SendDeleteAllBondsRequest(void)
{
    if (MessageProtocol_IsIdle()) {
        Log_Debug("INFO: Sending \"Delete all BLE bonds\" request.\n");
        MessageProtocol_SendRequest(MessageProtocol_BleControlCategoryId,
                                    BleControlMessageProtocol_DeleteAllBleBondsRequestId, NULL, 0,
                                    SendDeleteAllBondsResponseHandler);
        deleteAllBleBondsDeviceRequired = false;
    } else {
        deleteAllBleBondsDeviceRequired = true;
    }
}

static void BleDeviceUpEventHandler(MessageProtocol_CategoryId categoryId,
                                    MessageProtocol_EventId requestId)
{
    // Reset state because nRF52 has just rebooted.
    currentAdvertisingMode = BleControlMessageProtocol_NotAdvertisingMode;
    initializeDeviceRequired = false;
    setPasskeyRequired = false;
    changeBleAdvertisingModeRequired = false;
    deleteAllBleBondsDeviceRequired = false;
    struct timespec disabled = {0, 0};
    SetTimerFdToPeriod(bleAdvertiseToAllTimerFd, &disabled);

    // Start to initialize nRF52.
    SendInitializeBleDeviceRequest();
    ChangeBleProtocolState(BleControlMessageProtocolState_Uninitialized);
}

static void BleDeviceConnectedEventHandler(MessageProtocol_CategoryId categoryId,
                                           MessageProtocol_EventId requestId)
{
    if (blePublicState != BleControlMessageProtocolState_Error &&
        blePublicState != BleControlMessageProtocolState_Uninitialized) {
        Log_Debug("INFO: Received BLE connection event.\n");
        if (currentAdvertisingMode == BleControlMessageProtocol_AdvertisingToAllMode) {
            Log_Debug("INFO: Disabling advertising to all.\n");
            struct timespec disabled = {0, 0};
            SetTimerFdToPeriod(bleAdvertiseToAllTimerFd, &disabled);
            currentAdvertisingMode = BleControlMessageProtocol_AdvertisingToBondedDevicesMode;
        }

        ChangeBleProtocolState(BleControlMessageProtocolState_DeviceConnected);
    } else {
        Log_Debug("INFO: Received unexpected BLE connection event.\n");
    }
}

static void BleDeviceDisconnectedEventHandler(MessageProtocol_CategoryId categoryId,
                                              MessageProtocol_EventId requestId)
{
    if (blePublicState != BleControlMessageProtocolState_Error &&
        blePublicState != BleControlMessageProtocolState_Uninitialized) {
        Log_Debug("INFO: Received BLE disconnection event.\n");

        // While entering AdvertiseToAll state, a disconnect event can be triggered by an existing
        // connection being closed, and should be ignored.
        if (blePublicState != BleControlMessageProtocolState_AdvertisingToAllDevices) {
            ChangeBleProtocolState(BleControlMessageProtocolState_AdvertiseToBondedDevices);
        }
    } else {
        Log_Debug("INFO: Received unexpected BLE disconnection event.\n");
    }
}

static void DisplayPasskeyNeededEventHandler(MessageProtocol_CategoryId categoryId,
                                             MessageProtocol_EventId requestId)
{
    Log_Debug("INFO: A BLE central device is pairing and requires passkey: \"%s\".\n", blePasskey);
}

static void IdleHandler(void)
{
    if (initializeDeviceRequired) {
        SendInitializeBleDeviceRequest();
    }
    if (setPasskeyRequired) {
        SendSetPasskeyRequest();
    }
    if (changeBleAdvertisingModeRequired) {
        SendChangeBleAdvertisingModeRequest(desiredAdvertisingMode);
    }
    if (deleteAllBleBondsDeviceRequired) {
        SendDeleteAllBondsRequest();
    }
}

static void BleAdvertiseToAllTimeoutEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(bleAdvertiseToAllTimerFd) != 0) {
        return;
    }

    Log_Debug("INFO: BLE device advertising to all timeout reached.\n");
    SendChangeBleAdvertisingModeRequest(BleControlMessageProtocol_AdvertisingToBondedDevicesMode);
}

static void ChangeBleProtocolState(BleControlMessageProtocolState state)
{
    if (blePublicState != state) {
        blePublicState = state;
        if (bleStateChangeHandler != NULL) {
            bleStateChangeHandler(blePublicState);
        }
    }
}

int BleControlMessageProtocol_Init(BleControlMessageProtocol_StateChangeHandlerType handler,
                                   int epollFd)
{
    bleStateChangeHandler = handler;
    GenerateRandomBleDeviceName();

    // Set up BLE advertising to all timer, for later use.
    struct timespec disabled = {0, 0};
    static EventData bleAdvertiseToAllTimeoutEventData = {
        .eventHandler = &BleAdvertiseToAllTimeoutEventHandler};
    bleAdvertiseToAllTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &disabled, &bleAdvertiseToAllTimeoutEventData, EPOLLIN);
    if (bleAdvertiseToAllTimerFd == -1) {
        return -1;
    }

    MessageProtocol_RegisterEventHandler(MessageProtocol_BleControlCategoryId,
                                         BleControlMessageProtocol_BleDeviceUpEventId,
                                         BleDeviceUpEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_BleControlCategoryId,
                                         BleControlMessageProtocol_BleDeviceConnectedEventId,
                                         BleDeviceConnectedEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_BleControlCategoryId,
                                         BleControlMessageProtocol_BleDeviceDisconnectedEventId,
                                         BleDeviceDisconnectedEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_BleControlCategoryId,
                                         BleControlMessageProtocol_DisplayPasskeyNeededEventId,
                                         DisplayPasskeyNeededEventHandler);
    MessageProtocol_RegisterIdleHandler(IdleHandler);

    initializeDeviceRequired = false;
    setPasskeyRequired = false;
    changeBleAdvertisingModeRequired = false;
    deleteAllBleBondsDeviceRequired = false;
    currentAdvertisingMode = BleControlMessageProtocol_NotAdvertisingMode;
    blePublicState = BleControlMessageProtocolState_Uninitialized;

    // The Device Up event will kick off the initialization process.
    return 0;
}

void BleControlMessageProtocol_Cleanup(void)
{
    CloseFdAndPrintError(bleAdvertiseToAllTimerFd, "BleAdvertiseToAllTimer");
}

int BleControlMessageProtocol_AllowNewBleBond(struct timespec *timeout)
{
    if (blePublicState == BleControlMessageProtocolState_Error ||
        blePublicState == BleControlMessageProtocolState_Uninitialized) {
        return -1;
    }

    // Start (or restart) timer, after which the BLE device will start advertising to bonded
    // devices.
    SetTimerFdToSingleExpiry(bleAdvertiseToAllTimerFd, timeout);
    SendChangeBleAdvertisingModeRequest(BleControlMessageProtocol_AdvertisingToAllMode);
    return 0;
}

int BleControlMessageProtocol_DeleteAllBondedDevices(void)
{
    if (blePublicState == BleControlMessageProtocolState_Error ||
        blePublicState == BleControlMessageProtocolState_Uninitialized) {
        return -1;
    }

    SendDeleteAllBondsRequest();
    return 0;
}