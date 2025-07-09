/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <inttypes.h>

/// <summary>Request ID for a Get New Wi-Fi Details request message.</summary>
static const MessageProtocol_RequestId WifiConfigureMessageProtocol_GetNewWifiDetailsRequestId =
    0x0001;

/// <summary>Request ID for a Set Wi-Fi Scan Results Summary request message.</summary>
static const MessageProtocol_RequestId
    WifiConfigureMessageProtocol_SetWifiScanResultsSummaryRequestId = 0x0002;

/// <summary>Request ID for a Set Wi-Fi Status request message.</summary>
static const MessageProtocol_RequestId WifiConfigureMessageProtocol_SetWifiStatusRequestId = 0x0003;

/// <summary>Request ID for a Set Wi-Fi Operation Result request message.</summary>
static const MessageProtocol_RequestId
    WifiConfigureMessageProtocol_SetWifiOperationResultRequestId = 0x0004;

/// <summary>Request ID for a Set Next Wi-Fi Scan Result request message.</summary>
static const MessageProtocol_RequestId WifiConfigureMessageProtocol_SetNextWiFiScanResultRequestId =
    0x0005;

/// <summary>Event ID for a New Wi-Fi Details Available event message.</summary>
static const MessageProtocol_EventId WifiConfigureMessageProtocol_NewWiFiDetailsAvailableEventId =
    0x0001;

/// <summary>Event ID for a Wi-Fi Status Needed event message.</summary>
static const MessageProtocol_EventId WifiConfigureMessageProtocol_WifiStatusNeededEventId = 0x0002;

/// <summary>Event ID for a Wi-Fi Scan Needed event message.</summary>
static const MessageProtocol_EventId WifiConfigureMessageProtocol_WifiScanNeededEventId = 0x0003;

/// <summary>A connection status value indicating no network connection is available.</summary>
static const uint8_t WifiConfigureMessageProtocol_NoConnection = 0x00;

/// <summary>
///     A connection status value indicating that Wi-Fi is connected but no internet
///     connection is available.
/// </summary>
static const uint8_t WifiConfigureMessageProtocol_WifiConnected = 0x01 << 0;

/// <summary>
///     A connection status value indicating that Wi-Fi is connected and a full internet
///     connection is available.
/// </summary>
static const uint8_t WifiConfigureMessageProtocol_InternetConnected = 0x01 << 1;

/// <summary>
///     A connection status value indicating that Wi-Fi has an IP address assigned to it.
/// </summary>
static const uint8_t WifiConfigureMessageProtocol_IpAddressAvailable = 0x01 << 2;

/// <summary>
///     Data structure for the body of a
///     <see cref="WifiConfigureMessageProtocol_GetNewWifiDetailsRequestId" /> response message.
///     This structure describes a found Wi-Fi network.
/// </summary>
typedef struct {
    /// <summary>
    ///     Security type as defined by <see cref="WifiConfig_Security" /> in applibs/wificonfig.h.
    /// </summary>
    uint8_t securityType;
    /// <summary>Length (in bytes) of the SSID for this network.</summary>
    uint8_t ssidLength;
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved1[2];
    /// <summary>The SSID for this network, as a fixed-length array of bytes.</summary>
    uint8_t ssid[32];
    /// <summary>
    ///     The length (in bytes) of the PSK for this network.
    ///     Should be 0 if this is an open network.
    /// </summary>
    uint8_t pskLength;
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved2[3];
    /// <summary>The PSK for this network, if required. Not needed for an open network.</summary>
    uint8_t psk[64];
    /// <summary>Whether this network should be searched for by name. 1 for yes; 0 for no.</summary>
    uint8_t targetedScan;
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved3[3];
} WifiConfigureMessageProtocol_NewWifiDetailsStruct;

/// <summary>
///     Data structure for the body of a
///     <see cref="WifiConfigureMessageProtocol_SetWifiStatusRequestId" /> request message. This
///     structure describes the current status of the Wi-Fi connection on the Azure Sphere device.
/// </summary>
typedef struct {
    /// <summary>
    ///     The connection status - one of <see cref="WifiConfigureMessageProtocol_NoConnection" />,
    ///     <see cref="WifiConfigureMessageProtocol_WifiConnected" /> or
    ///     <see cref="WifiConfigureMessageProtocol_InternetConnected" />
    /// </summary>
    uint8_t connectionStatus;
    /// <summary>The RSSI (Received Signal Strength Indicator) value.</summary>
    int8_t signalLevel;
    /// <summary>
    ///     Security type as defined by <see cref="WifiConfig_Security" /> in
    ///     applibs/wificonfig.h.</summary>
    /// </summary>
    uint8_t securityType;
    /// <summary>Length (in bytes) of the SSID for this network.</summary>
    uint8_t ssidLength;
    /// <summary>The SSID for this network, as a fixed-length array of bytes.</summary>
    uint8_t ssid[32];
    /// <summary>The BSS center frequency in MHz.</summary>
    uint32_t frequency;
    /// <summary>The BSSID for this network, as a fixed-length array of bytes.</summary>
    uint8_t bssid[6];
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved[2];
} WifiConfigureMessageProtocol_WifiStatusRequestStruct;

/// <summary>
///     Data structure for the body of a
///     <see cref="WifiConfigureMessageProtocol_SetWifiScanResultsSummaryRequestId"/> request
///     message.
/// </summary>
typedef struct {
    /// <summary>
    ///     The result of the scan - 0 indicates success; any other value indicates an
    ///     error.
    /// </summary>
    uint8_t scanResult;
    /// <summary>The number of networks found during the scan.</summary>
    uint8_t totalNetworkCount;
    /// <summary>Reserved; must all be 0.</summary>
    uint8_t reserved[2];
    /// <summary>Number of bytes required to store the results of the whole scan.</summary>
    uint32_t totalResultsSize;
} WifiConfigureMessageProtocol_WifiScanResultsSummaryRequestStruct;

/// <summary>
///     Data structure for the body of a
///     <see cref="WifiConfigureMessageProtocol_SetNextWiFiScanResultRequestId"/> request
///     message. This structure describes a network as found during a network scan.
/// </summary>
typedef struct {
    /// <summary>
    ///     Security type as defined by <see cref="WifiConfig_Security" /> in
    ///     applibs/wificonfig.h.</summary>
    /// </summary>
    uint8_t securityType;
    /// <summary>The RSSI (Received Signal Strength Indicator) value.</summary>
    int8_t signalRssi;
    /// <summary>Length (in bytes) of the SSID for this network.</summary>
    uint8_t ssidLength;
    /// <summary>Reserved; must be 0.</summary>
    uint8_t reserved;
    /// <summary>The SSID for this network, as a fixed-length array of bytes.</summary>
    uint8_t ssid[32];
} WifiConfigureMessageProtocol_WifiScanResultRequestStruct;
