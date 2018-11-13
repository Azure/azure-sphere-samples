/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "wificonfig_message_protocol.h"
#include "wificonfig_message_protocol_defs.h"
#include "message_protocol.h"
#include "applibs_versions.h"
#include <applibs/wificonfig.h>
#include <applibs/networking.h>
#include <applibs/log.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

static bool newWiFiDetailsAvailableRequestNeeded;
static bool setWifiStatusRequestNeeded;
static bool setWifiScanResultsSummaryRequestNeeded;

#define MAX_AP_COUNT_FOUND_BY_SCAN 20
static WifiConfigureMessageProtocol_WifiScanResultRequestStruct
    foundAPs[MAX_AP_COUNT_FOUND_BY_SCAN];
static uint8_t foundAccessPointsCount = 0;
static uint8_t currentAccessPointIndex = 0;

// Wi-Fi response handlers
static void SetWifiOperationResultResponseHandler(MessageProtocol_CategoryId categoryId,
                                                  MessageProtocol_RequestId requestId,
                                                  const uint8_t *data, size_t dataSize,
                                                  MessageProtocol_ResponseResult result,
                                                  bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Set Wi-Fi Operation Result\" response.\n");
        return;
    }

    // This response contains no data, check its result to see whether the request is successful
    if (result != 0) {
        Log_Debug("ERROR: \"Set Wi-Fi Operation Result\" failed with error code: %d.\n", result);
        return;
    }
    Log_Debug("INFO: \"Set Wi-Fi Operation Result\" succeeded.\n");
}

static void GetNewWifiDetailsResponseHandler(MessageProtocol_CategoryId categoryId,
                                             MessageProtocol_RequestId requestId,
                                             const uint8_t *data, size_t dataSize,
                                             MessageProtocol_ResponseResult result, bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Get New Wi-Fi Details\" response.\n");
        return;
    }

    if (result != 0) {
        Log_Debug("ERROR: \"Get New Wi-Fi Details\" failed with error code: %d.\n", result);
        return;
    }

    if (dataSize != sizeof(WifiConfigureMessageProtocol_NewWifiDetailsStruct)) {
        Log_Debug("INFO: \"Get New Wi-Fi Details\" response is invalid.\n");
        return;
    }
    Log_Debug("INFO: \"Get New Wi-Fi Details\" succeeded.\n");

    WifiConfigureMessageProtocol_NewWifiDetailsStruct *newWifiDetails =
        (WifiConfigureMessageProtocol_NewWifiDetailsStruct *)data;

    // Store the new Wi-Fi network
    int wifiConfigResult = 0;
    if (newWifiDetails->securityType == WifiConfig_Security_Open) {
        wifiConfigResult =
            WifiConfig_StoreOpenNetwork(newWifiDetails->ssid, newWifiDetails->ssidLength);
    } else {
        wifiConfigResult =
            WifiConfig_StoreWpa2Network(newWifiDetails->ssid, newWifiDetails->ssidLength,
                                        newWifiDetails->psk, newWifiDetails->pskLength);
    }

    if (wifiConfigResult == 0) {
        Log_Debug("INFO: Wi-Fi network details stored successfully.\n");
    } else {
        Log_Debug("ERROR: Store Wi-Fi network failed: %s (%d).\n", strerror(errno), errno);
    }

    // Send "Set Wi-Fi Operation Result" message
    uint32_t resultCode = (wifiConfigResult == 0) ? 0 : (uint32_t)errno;
    Log_Debug("INFO: Sending request: \"Set Wi-Fi Operation Result\".\n");
    MessageProtocol_SendRequest(MessageProtocol_WifiConfigCategoryId,
                                WifiConfigureMessageProtocol_SetWifiOperationResultRequestId,
                                (const uint8_t *)&resultCode, sizeof(resultCode),
                                &SetWifiOperationResultResponseHandler);
}

static void SendSetNextWiFiScanResultRequest(void);

static void SetWifiScanResultsSummaryResponseHandler(MessageProtocol_CategoryId categoryId,
                                                     MessageProtocol_RequestId requestId,
                                                     const uint8_t *data, size_t dataSize,
                                                     MessageProtocol_ResponseResult result,
                                                     bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Set Wi-Fi Scan Results Summary\" response.\n");
        foundAccessPointsCount = 0;
        return;
    }

    // This response contains no data, check its result to see whether the request is successful
    if (result != 0) {
        Log_Debug("ERROR: \"Set Wi-Fi Scan Results Summary\" failed with error code: %d.\n",
                  result);
        return;
    }
    Log_Debug("INFO: \"Set Wi-Fi Scan Results Summary\" succeeded.\n");

    if (foundAccessPointsCount > 0) {
        SendSetNextWiFiScanResultRequest();
    }
}

static void SetWifiStatusResponseHandler(MessageProtocol_CategoryId categoryId,
                                         MessageProtocol_RequestId requestId, const uint8_t *data,
                                         size_t dataSize, MessageProtocol_ResponseResult result,
                                         bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Set Wi-Fi Status\" response.\n");
        return;
    }

    // This response contains no data, check its result to see whether the request is successful
    if (result != 0) {
        Log_Debug("ERROR: \"Set Wi-Fi Status\" failed with error code: %d.\n", result);
        return;
    }
    Log_Debug("INFO: \"Set Wi-Fi Status\" succeeded.\n");
}

static void SetNextWifiScanResultResponseHandler(MessageProtocol_CategoryId categoryId,
                                                 MessageProtocol_RequestId requestId,
                                                 const uint8_t *data, size_t dataSize,
                                                 MessageProtocol_ResponseResult result,
                                                 bool timedOut)
{
    if (timedOut) {
        Log_Debug("ERROR: Timed out waiting for \"Set Next Wi-Fi Scan Result\" response.\n");
        foundAccessPointsCount = 0;
        currentAccessPointIndex = 0;
        return;
    }

    // This response contains no data, check its result to see whether the request is successful
    if (result != 0) {
        Log_Debug("ERROR: \"Set Next Wi-Fi Scan Result\" failed with error code: %d.\n", result);
        return;
    }
    Log_Debug("INFO: \"Set Next Wi-Fi Scan Result\" succeeded.\n");
    if (foundAccessPointsCount > 0 && currentAccessPointIndex < foundAccessPointsCount) {
        SendSetNextWiFiScanResultRequest();
    }
}

static void SendNewWifiDetailsRequest(void)
{
    newWiFiDetailsAvailableRequestNeeded = false;
    // Send get new Wi-Fi details request, with empty data
    Log_Debug("INFO: Sending request: \"Get New Wi-Fi Details\".\n");
    MessageProtocol_SendRequest(MessageProtocol_WifiConfigCategoryId,
                                WifiConfigureMessageProtocol_GetNewWifiDetailsRequestId, NULL, 0,
                                &GetNewWifiDetailsResponseHandler);
}

static void NewWifiDetailsAvailableEventHandler(MessageProtocol_CategoryId categoryId,
                                                MessageProtocol_EventId eventId)
{
    if (MessageProtocol_IsIdle()) {
        SendNewWifiDetailsRequest();
    } else {
        newWiFiDetailsAvailableRequestNeeded = true;
    }
}

static void SendSetWifiStatusRequest(void)
{
    Log_Debug("INFO: Handling event: \"Wi-Fi Status Needed\".\n");
    setWifiStatusRequestNeeded = false;

    // Get the current Wi-Fi status
    WifiConfigureMessageProtocol_WifiStatusRequestStruct wifiStatus;
    memset(&wifiStatus, 0, sizeof(wifiStatus));
    WifiConfig_ConnectedNetwork network;
    int result = WifiConfig_GetCurrentNetwork(&network);
    if (result == 0) {
        // If there is a current Wi-Fi network, get the network connection status
        bool connectedToInternet = false;
        result = Networking_IsNetworkingReady(&connectedToInternet);
        if (result != 0) {
            // If the call has failed, log the error and assume we are not connected to the
            // internet.
            Log_Debug("ERROR: Networking_IsNetworkingReady failed with error: %s (%d)", strerror(errno), errno);
            connectedToInternet = false;
        }

        // Populate the Wi-Fi status struct with the network status.
        wifiStatus.connectionStatus = WifiConfigureMessageProtocol_WifiConnected;
        if (connectedToInternet) {
            wifiStatus.connectionStatus |= WifiConfigureMessageProtocol_InternetConnected;
        }
        wifiStatus.signalLevel = network.signalRssi;
        wifiStatus.securityType = network.security;
        wifiStatus.ssidLength = network.ssidLength;
        memcpy(wifiStatus.ssid, network.ssid, network.ssidLength);
        wifiStatus.frequency = network.frequencyMHz;
        memcpy(wifiStatus.bssid, network.bssid, WIFICONFIG_BSSID_BUFFER_SIZE);
    } else {
        // There is no currently connected network
        wifiStatus.connectionStatus = WifiConfigureMessageProtocol_NoConnection;
    }

    // Finally, send a "Set Wi-Fi Status" request
    Log_Debug("INFO: Sending request: \"Set Wi-Fi Status\".\n");
    MessageProtocol_SendRequest(
        MessageProtocol_WifiConfigCategoryId, WifiConfigureMessageProtocol_SetWifiStatusRequestId,
        (const uint8_t *)&wifiStatus, sizeof(wifiStatus), &SetWifiStatusResponseHandler);
}

static void WifiStatusNeededEventHandler(MessageProtocol_CategoryId categoryId,
                                         MessageProtocol_EventId eventId)
{
    if (MessageProtocol_IsIdle()) {
        SendSetWifiStatusRequest();
    } else {
        setWifiStatusRequestNeeded = true;
    }
}

static bool IsSameAccessPoint(
    const WifiConfigureMessageProtocol_WifiScanResultRequestStruct *target,
    const WifiConfig_ScannedNetwork *source)
{
    if (target->securityType == source->security && target->ssidLength == source->ssidLength) {
        return 0 == memcmp(target->ssid, source->ssid, target->ssidLength);
    }
    return false;
}

static void SetScannedNetwork(WifiConfigureMessageProtocol_WifiScanResultRequestStruct *target,
                              const WifiConfig_ScannedNetwork *source)
{
    target->securityType = source->security;
    target->ssidLength = source->ssidLength;
    target->signalRssi = source->signalRssi;
    memcpy(target->ssid, source->ssid, target->ssidLength);
}

static uint8_t CollapseNetworks(const WifiConfig_ScannedNetwork *target, size_t count)
{
    uint8_t scannedNetworksCount = 0;
    for (size_t i = 0; i < count; ++i) {
        bool found = false;
        for (size_t j = 0; j < scannedNetworksCount; ++j) {
            if (IsSameAccessPoint(foundAPs + j, target + i)) {
                if (foundAPs[j].signalRssi < target[i].signalRssi) {
                    foundAPs[j].signalRssi = target[i].signalRssi;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            if (scannedNetworksCount >= MAX_AP_COUNT_FOUND_BY_SCAN) {
                Log_Debug("INFO: Returning only the first %d networks found by scan.\n",
                          MAX_AP_COUNT_FOUND_BY_SCAN);
                break;
            }
            SetScannedNetwork(foundAPs + scannedNetworksCount, target + i);
            ++scannedNetworksCount;
        }
    }
    return scannedNetworksCount;
}

static void SendSetWifiScanResultsSummaryRequestNeeded(void)
{
    setWifiScanResultsSummaryRequestNeeded = false;

    // Get Wi-Fi scan results count and populate found access points
    Log_Debug("INFO: Handle received event message: \"Wi-Fi Scan Needed\".\n");
    WifiConfigureMessageProtocol_WifiScanResultsSummaryRequestStruct scanSummary;
    uint8_t scanResult = 0;
    foundAccessPointsCount = 0;
    currentAccessPointIndex = 0;
    ssize_t result = WifiConfig_TriggerScanAndGetScannedNetworkCount();
    if (result < 0) {
        scanResult = 1;
        Log_Debug(
            "ERROR: Get scanned network count failed with error: %s (%d).\n",
            strerror(errno), errno);
    } else if (result == 0) {
        Log_Debug("INFO: Scan found no Wi-Fi networks.\n");
    } else {
        size_t networkCount = (size_t)result;
        WifiConfig_ScannedNetwork *networks =
            (WifiConfig_ScannedNetwork *)malloc(sizeof(WifiConfig_ScannedNetwork) * networkCount);
        ssize_t getScannedNetworksResult = WifiConfig_GetScannedNetworks(networks, networkCount);
        if (getScannedNetworksResult < 0) {
            scanResult = 2;
            Log_Debug("ERROR: Get scanned networks failed with error: %s (%d).\n",
                      strerror(errno), errno);
        } else {
            // Collapse all the found networks to access points based on SSID and Security Type
            foundAccessPointsCount = CollapseNetworks(networks, (size_t)getScannedNetworksResult);
            Log_Debug("Scan found %d Wi-Fi networks.\n", foundAccessPointsCount);
        }
        free(networks);
    }

    // Populate the scan summary response struct
    scanSummary.scanResult = scanResult;
    scanSummary.totalNetworkCount = foundAccessPointsCount;
    scanSummary.totalResultsSize = foundAccessPointsCount * sizeof(scanSummary);

    // Send "Set Wi-Fi Scan Results Summary" request
    Log_Debug("INFO: Sending request: \"Set Wi-Fi Scan Results Summary\".\n");
    MessageProtocol_SendRequest(MessageProtocol_WifiConfigCategoryId,
                                WifiConfigureMessageProtocol_SetWifiScanResultsSummaryRequestId,
                                (const uint8_t *)&scanSummary, sizeof(scanSummary),
                                &SetWifiScanResultsSummaryResponseHandler);
}

static void SendSetNextWiFiScanResultRequest(void)
{
    if (currentAccessPointIndex < foundAccessPointsCount) {
        Log_Debug("INFO: Sending request: \"Set Next Wi-Fi Scan Result\" (%d).\n",
                  currentAccessPointIndex);
        MessageProtocol_SendRequest(
            MessageProtocol_WifiConfigCategoryId,
            WifiConfigureMessageProtocol_SetNextWiFiScanResultRequestId,
            (const uint8_t *)&foundAPs[currentAccessPointIndex],
            sizeof(WifiConfigureMessageProtocol_WifiScanResultRequestStruct),
            &SetNextWifiScanResultResponseHandler);
        ++currentAccessPointIndex;
    } else {
        Log_Debug("ERROR: Invalid index (%d) for scanned network result.\n",
                  currentAccessPointIndex);
        currentAccessPointIndex = 0;
    }
}

static void WifiScanNeededEventHandler(MessageProtocol_CategoryId categoryId,
                                       MessageProtocol_EventId eventId)
{
    if (MessageProtocol_IsIdle()) {
        SendSetWifiScanResultsSummaryRequestNeeded();
    } else {
        setWifiScanResultsSummaryRequestNeeded = true;
    }
}

static void IdleHandler(void)
{
    if (newWiFiDetailsAvailableRequestNeeded) {
        SendNewWifiDetailsRequest();
        return;
    }
    if (setWifiStatusRequestNeeded) {
        SendSetWifiStatusRequest();
        return;
    }
    if (setWifiScanResultsSummaryRequestNeeded) {
        SendSetWifiScanResultsSummaryRequestNeeded();
        return;
    }
}

void WifiConfigMessageProtocol_Init(void)
{
    // Register event handlers
    MessageProtocol_RegisterEventHandler(
        MessageProtocol_WifiConfigCategoryId,
        WifiConfigureMessageProtocol_NewWiFiDetailsAvailableEventId,
        NewWifiDetailsAvailableEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_WifiConfigCategoryId,
                                         WifiConfigureMessageProtocol_WifiStatusNeededEventId,
                                         WifiStatusNeededEventHandler);
    MessageProtocol_RegisterEventHandler(MessageProtocol_WifiConfigCategoryId,
                                         WifiConfigureMessageProtocol_WifiScanNeededEventId,
                                         WifiScanNeededEventHandler);

    // Register idle handlers
    MessageProtocol_RegisterIdleHandler(IdleHandler);

    // Initialize event pending flags
    newWiFiDetailsAvailableRequestNeeded = false;
    setWifiStatusRequestNeeded = false;
    setWifiScanResultsSummaryRequestNeeded = false;
}

void WifiConfigMessageProtocol_Cleanup(void) {}
