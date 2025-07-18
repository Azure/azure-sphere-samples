/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "mqtt_client.h"
#include <applibs/wificonfig.h>
#include <applibs/networking.h>

// WiFi Monitor Configuration
typedef struct {
    int intervalMinutes;            // 5, 10, or 15 minutes
    int signalThreshold;            // RSSI threshold for weak signal alerts (-70 dBm default)
    int reconnectThreshold;         // Max reconnects per hour before alert
    bool alertOnDisconnect;         // Send alert immediately on disconnect
    bool alertOnWeakSignal;         // Send alert on weak signal
    bool alertOnFrequentReconnects; // Send alert on frequent reconnects
} WiFiMonitorConfig;

// WiFi Monitor State
typedef struct {
    WiFiStatusRecord currentStatus;
    WiFiStatusRecord history[5];    // Last 5 status checks
    int historyIndex;
    int historyCount;
    bool hasDropped;
    time_t lastCheck;
    time_t lastAlert;
    int reconnectCount;
    time_t reconnectTimes[10];      // Track last 10 reconnect times
    int reconnectIndex;
    WiFiMonitorConfig config;
} WiFiMonitorState;

/// <summary>
/// Initialize WiFi monitoring system
/// </summary>
/// <param name="monitor">WiFi monitor state structure</param>
/// <param name="config">Monitor configuration</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_Initialize(WiFiMonitorState* monitor, const WiFiMonitorConfig* config);

/// <summary>
/// Perform WiFi status check and update monitor state
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_PerformCheck(WiFiMonitorState* monitor);

/// <summary>
/// Check for alert conditions and return alert type if any
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="alertType">Output buffer for alert type string</param>
/// <param name="alertTypeSize">Size of alert type buffer</param>
/// <returns>true if alert should be sent, false otherwise</returns>
bool WiFiMonitor_CheckAlerts(WiFiMonitorState* monitor, char* alertType, size_t alertTypeSize);

/// <summary>
/// Get JSON status message for MQTT publishing
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="deviceConfig">Device configuration for device info</param>
/// <param name="jsonBuffer">Output buffer for JSON message</param>
/// <param name="bufferSize">Size of JSON buffer</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_GetStatusJSON(const WiFiMonitorState* monitor, const DeviceConfig* deviceConfig,
                             char* jsonBuffer, size_t bufferSize);

/// <summary>
/// Get JSON alert message for MQTT publishing
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="deviceConfig">Device configuration for device info</param>
/// <param name="alertType">Type of alert</param>
/// <param name="jsonBuffer">Output buffer for JSON message</param>
/// <param name="bufferSize">Size of JSON buffer</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_GetAlertJSON(const WiFiMonitorState* monitor, const DeviceConfig* deviceConfig,
                            const char* alertType, char* jsonBuffer, size_t bufferSize);

/// <summary>
/// Get JSON heartbeat message for MQTT publishing
/// </summary>
/// <param name="deviceConfig">Device configuration</param>
/// <param name="jsonBuffer">Output buffer for JSON message</param>
/// <param name="bufferSize">Size of JSON buffer</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_GetHeartbeatJSON(const DeviceConfig* deviceConfig, char* jsonBuffer, size_t bufferSize);

/// <summary>
/// Handle configuration update from MQTT
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="configJSON">JSON configuration string</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_HandleConfigUpdate(WiFiMonitorState* monitor, const char* configJSON);

/// <summary>
/// Handle command from MQTT
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="commandJSON">JSON command string</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_HandleCommand(WiFiMonitorState* monitor, const char* commandJSON);

/// <summary>
/// Record a reconnection event
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
void WiFiMonitor_RecordReconnect(WiFiMonitorState* monitor);

/// <summary>
/// Get number of reconnects in time window (in seconds)
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="windowSeconds">Time window in seconds</param>
/// <returns>Number of reconnects in the specified time window</returns>
int WiFiMonitor_GetReconnectsInWindow(const WiFiMonitorState* monitor, int windowSeconds);

/// <summary>
/// Get current WiFi monitor status for debugging
/// </summary>
/// <param name="monitor">WiFi monitor state</param>
/// <param name="statusBuffer">Output buffer for status string</param>
/// <param name="bufferSize">Size of status buffer</param>
/// <returns>0 on success, -1 on failure</returns>
int WiFiMonitor_GetStatusString(const WiFiMonitorState* monitor, char* statusBuffer, size_t bufferSize);