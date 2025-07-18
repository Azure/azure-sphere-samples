/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "wifi_monitor.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <applibs/log.h>
#include <applibs/wificonfig.h>
#include <applibs/networking.h>

// JSON formatting helpers
#define JSON_BUFFER_SIZE 1024

// Helper function to get free memory (simplified for Azure Sphere)
static int GetFreeMemoryKB(void) {
    // Azure Sphere doesn't have a simple way to get free memory
    // This is a placeholder - you might need to implement this differently
    return 1024; // Return a default value
}

// Helper function to format SSID safely
static void FormatSSIDSafe(const uint8_t* ssid, size_t ssidLength, char* output, size_t outputSize) {
    if (!ssid || !output || outputSize == 0) {
        return;
    }
    
    size_t copyLength = (ssidLength < outputSize - 1) ? ssidLength : (outputSize - 1);
    
    for (size_t i = 0; i < copyLength; i++) {
        // Replace non-printable characters with '.'
        output[i] = (ssid[i] >= 32 && ssid[i] <= 126) ? (char)ssid[i] : '.';
    }
    output[copyLength] = '\0';
}

/// <summary>
/// Initialize WiFi monitoring system
/// </summary>
int WiFiMonitor_Initialize(WiFiMonitorState* monitor, const WiFiMonitorConfig* config) {
    if (!monitor || !config) {
        Log_Debug("ERROR: Invalid parameters for WiFiMonitor_Initialize\n");
        return -1;
    }
    
    memset(monitor, 0, sizeof(WiFiMonitorState));
    monitor->config = *config;
    monitor->historyIndex = 0;
    monitor->historyCount = 0;
    monitor->reconnectIndex = 0;
    monitor->lastCheck = 0;
    monitor->lastAlert = 0;
    
    Log_Debug("INFO: WiFi Monitor initialized - interval: %d minutes, signal threshold: %d dBm\n",
              config->intervalMinutes, config->signalThreshold);
    
    return 0;
}

/// <summary>
/// Perform WiFi status check and update monitor state
/// </summary>
int WiFiMonitor_PerformCheck(WiFiMonitorState* monitor) {
    if (!monitor) {
        return -1;
    }
    
    WiFiStatusRecord record = {0};
    record.timestamp = time(NULL);
    monitor->lastCheck = record.timestamp;
    
    // Check if networking is ready
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == 0) {
        record.isNetworkReady = isNetworkReady;
    } else {
        record.isNetworkReady = false;
        snprintf(record.errorMessage, sizeof(record.errorMessage), 
                "Failed to check network readiness");
    }
    
    // Get WiFi connection status
    if (record.isNetworkReady) {
        WifiConfig_ConnectedNetwork connectedNetwork;
        int result = WifiConfig_GetCurrentNetwork(&connectedNetwork);
        
        if (result == 0) {
            record.isConnected = true;
            record.signalStrength = connectedNetwork.signalRssi;
            FormatSSIDSafe(connectedNetwork.ssid, connectedNetwork.ssidLength, 
                          record.ssid, sizeof(record.ssid));
        } else if (result == -1 && errno == ENODATA) {
            record.isConnected = false;
            snprintf(record.errorMessage, sizeof(record.errorMessage), 
                    "Not connected to any WiFi network");
        } else {
            record.isConnected = false;
            snprintf(record.errorMessage, sizeof(record.errorMessage), 
                    "Failed to get current network: %s", strerror(errno));
        }
    }
    
    // Calculate uptime
    record.uptimeHours = MQTT_GetUptimeHours();
    
    // Update history
    monitor->history[monitor->historyIndex] = record;
    monitor->historyIndex = (monitor->historyIndex + 1) % 5;
    if (monitor->historyCount < 5) {
        monitor->historyCount++;
    }
    
    // Update current status
    monitor->currentStatus = record;
    
    // Check for connection drop
    if (monitor->historyCount > 1) {
        int prevIndex = (monitor->historyIndex - 2 + 5) % 5;
        if (monitor->history[prevIndex].isConnected && !record.isConnected) {
            monitor->hasDropped = true;
            Log_Debug("INFO: WiFi connection drop detected\n");
        }
    }
    
    Log_Debug("INFO: WiFi Check - Connected: %s, Signal: %d dBm, SSID: %s\n",
              record.isConnected ? "Yes" : "No", 
              record.signalStrength, 
              record.ssid);
    
    return 0;
}

/// <summary>
/// Check for alert conditions
/// </summary>
bool WiFiMonitor_CheckAlerts(WiFiMonitorState* monitor, char* alertType, size_t alertTypeSize) {
    if (!monitor || !alertType || alertTypeSize == 0) {
        return false;
    }
    
    time_t currentTime = time(NULL);
    
    // Rate limit alerts - don't send more than one per 5 minutes
    if (currentTime - monitor->lastAlert < 300) {
        return false;
    }
    
    // Check for connection lost
    if (monitor->config.alertOnDisconnect && monitor->hasDropped) {
        strncpy(alertType, "connection_lost", alertTypeSize - 1);
        alertType[alertTypeSize - 1] = '\0';
        monitor->hasDropped = false; // Reset flag
        monitor->lastAlert = currentTime;
        return true;
    }
    
    // Check for weak signal
    if (monitor->config.alertOnWeakSignal && 
        monitor->currentStatus.isConnected &&
        monitor->currentStatus.signalStrength < monitor->config.signalThreshold) {
        strncpy(alertType, "signal_weak", alertTypeSize - 1);
        alertType[alertTypeSize - 1] = '\0';
        monitor->lastAlert = currentTime;
        return true;
    }
    
    // Check for frequent reconnects
    if (monitor->config.alertOnFrequentReconnects) {
        int recentReconnects = WiFiMonitor_GetReconnectsInWindow(monitor, 3600); // Last hour
        if (recentReconnects >= monitor->config.reconnectThreshold) {
            strncpy(alertType, "frequent_reconnects", alertTypeSize - 1);
            alertType[alertTypeSize - 1] = '\0';
            monitor->lastAlert = currentTime;
            return true;
        }
    }
    
    return false;
}

/// <summary>
/// Get JSON status message for MQTT publishing
/// </summary>
int WiFiMonitor_GetStatusJSON(const WiFiMonitorState* monitor, const DeviceConfig* deviceConfig,
                             char* jsonBuffer, size_t bufferSize) {
    if (!monitor || !deviceConfig || !jsonBuffer || bufferSize == 0) {
        return -1;
    }
    
    const WiFiStatusRecord* status = &monitor->currentStatus;
    
    int result = snprintf(jsonBuffer, bufferSize,
        "{"
        "\"device_id\":\"%s\","
        "\"location\":\"%s\","
        "\"type\":\"wifi_status\","
        "\"timestamp\":%ld,"
        "\"connected\":%s,"
        "\"network_ready\":%s,"
        "\"signal_strength\":%d,"
        "\"ssid\":\"%s\","
        "\"fw_version\":\"%s\","
        "\"uptime_hours\":%.2f,"
        "\"reconnect_count\":%d"
        "}",
        deviceConfig->DEVICE_ID,
        deviceConfig->LOCATION,
        status->timestamp,
        status->isConnected ? "true" : "false",
        status->isNetworkReady ? "true" : "false",
        status->signalStrength,
        status->ssid,
        deviceConfig->FW_VERSION,
        status->uptimeHours,
        monitor->reconnectCount
    );
    
    if (result < 0 || (size_t)result >= bufferSize) {
        Log_Debug("ERROR: JSON buffer too small for status message\n");
        return -1;
    }
    
    return 0;
}

/// <summary>
/// Get JSON alert message for MQTT publishing
/// </summary>
int WiFiMonitor_GetAlertJSON(const WiFiMonitorState* monitor, const DeviceConfig* deviceConfig,
                            const char* alertType, char* jsonBuffer, size_t bufferSize) {
    if (!monitor || !deviceConfig || !alertType || !jsonBuffer || bufferSize == 0) {
        return -1;
    }
    
    const WiFiStatusRecord* status = &monitor->currentStatus;
    
    int result = snprintf(jsonBuffer, bufferSize,
        "{"
        "\"device_id\":\"%s\","
        "\"location\":\"%s\","
        "\"type\":\"wifi_alert\","
        "\"alert_type\":\"%s\","
        "\"timestamp\":%ld,"
        "\"signal_strength\":%d,"
        "\"ssid\":\"%s\","
        "\"connected\":%s,"
        "\"reconnect_count\":%d,"
        "\"uptime_hours\":%.2f"
        "}",
        deviceConfig->DEVICE_ID,
        deviceConfig->LOCATION,
        alertType,
        status->timestamp,
        status->signalStrength,
        status->ssid,
        status->isConnected ? "true" : "false",
        monitor->reconnectCount,
        status->uptimeHours
    );
    
    if (result < 0 || (size_t)result >= bufferSize) {
        Log_Debug("ERROR: JSON buffer too small for alert message\n");
        return -1;
    }
    
    return 0;
}

/// <summary>
/// Get JSON heartbeat message for MQTT publishing
/// </summary>
int WiFiMonitor_GetHeartbeatJSON(const DeviceConfig* deviceConfig, char* jsonBuffer, size_t bufferSize) {
    if (!deviceConfig || !jsonBuffer || bufferSize == 0) {
        return -1;
    }
    
    int result = snprintf(jsonBuffer, bufferSize,
        "{"
        "\"device_id\":\"%s\","
        "\"location\":\"%s\","
        "\"status\":\"alive\","
        "\"fw_version\":\"%s\","
        "\"timestamp\":%ld,"
        "\"uptime_hours\":%.2f,"
        "\"free_memory_kb\":%d"
        "}",
        deviceConfig->DEVICE_ID,
        deviceConfig->LOCATION,
        deviceConfig->FW_VERSION,
        time(NULL),
        MQTT_GetUptimeHours(),
        GetFreeMemoryKB()
    );
    
    if (result < 0 || (size_t)result >= bufferSize) {
        Log_Debug("ERROR: JSON buffer too small for heartbeat message\n");
        return -1;
    }
    
    return 0;
}

/// <summary>
/// Handle configuration update from MQTT
/// </summary>
int WiFiMonitor_HandleConfigUpdate(WiFiMonitorState* monitor, const char* configJSON) {
    if (!monitor || !configJSON) {
        return -1;
    }
    
    // Simple JSON parsing for configuration updates
    // In a production system, you'd use a proper JSON library
    
    // Check for monitor_interval
    char* intervalPtr = strstr(configJSON, "\"monitor_interval\":");
    if (intervalPtr) {
        int newInterval;
        if (sscanf(intervalPtr + 19, "%d", &newInterval) == 1) {
            if (newInterval == 5 || newInterval == 10 || newInterval == 15) {
                monitor->config.intervalMinutes = newInterval;
                Log_Debug("INFO: Monitor interval updated to %d minutes\n", newInterval);
            }
        }
    }
    
    // Check for debug flag
    char* debugPtr = strstr(configJSON, "\"debug\":");
    if (debugPtr) {
        if (strstr(debugPtr + 8, "true")) {
            Log_Debug("INFO: Debug enabled via MQTT config\n");
        } else if (strstr(debugPtr + 8, "false")) {
            Log_Debug("INFO: Debug disabled via MQTT config\n");
        }
    }
    
    // Check for signal threshold
    char* thresholdPtr = strstr(configJSON, "\"signal_threshold\":");
    if (thresholdPtr) {
        int newThreshold;
        if (sscanf(thresholdPtr + 19, "%d", &newThreshold) == 1) {
            if (newThreshold >= -100 && newThreshold <= -30) {
                monitor->config.signalThreshold = newThreshold;
                Log_Debug("INFO: Signal threshold updated to %d dBm\n", newThreshold);
            }
        }
    }
    
    return 0;
}

/// <summary>
/// Handle command from MQTT
/// </summary>
int WiFiMonitor_HandleCommand(WiFiMonitorState* monitor, const char* commandJSON) {
    if (!monitor || !commandJSON) {
        return -1;
    }
    
    // Simple command parsing
    if (strstr(commandJSON, "\"action\":\"force_check\"")) {
        Log_Debug("INFO: Force check command received\n");
        return WiFiMonitor_PerformCheck(monitor);
    }
    
    if (strstr(commandJSON, "\"action\":\"restart_monitor\"")) {
        Log_Debug("INFO: Restart monitor command received\n");
        // Reset monitor state
        monitor->historyIndex = 0;
        monitor->historyCount = 0;
        monitor->hasDropped = false;
        return 0;
    }
    
    if (strstr(commandJSON, "\"action\":\"get_status\"")) {
        Log_Debug("INFO: Get status command received\n");
        char statusBuffer[512];
        WiFiMonitor_GetStatusString(monitor, statusBuffer, sizeof(statusBuffer));
        Log_Debug("Current Status: %s\n", statusBuffer);
        return 0;
    }
    
    Log_Debug("INFO: Unknown command received: %s\n", commandJSON);
    return -1;
}

/// <summary>
/// Record a reconnection event
/// </summary>
void WiFiMonitor_RecordReconnect(WiFiMonitorState* monitor) {
    if (!monitor) {
        return;
    }
    
    time_t currentTime = time(NULL);
    monitor->reconnectTimes[monitor->reconnectIndex] = currentTime;
    monitor->reconnectIndex = (monitor->reconnectIndex + 1) % 10;
    monitor->reconnectCount++;
    
    Log_Debug("INFO: Reconnect recorded - total count: %d\n", monitor->reconnectCount);
}

/// <summary>
/// Get number of reconnects in time window
/// </summary>
int WiFiMonitor_GetReconnectsInWindow(const WiFiMonitorState* monitor, int windowSeconds) {
    if (!monitor) {
        return 0;
    }
    
    time_t currentTime = time(NULL);
    int count = 0;
    
    for (int i = 0; i < 10; i++) {
        if (monitor->reconnectTimes[i] != 0 && 
            (currentTime - monitor->reconnectTimes[i]) < windowSeconds) {
            count++;
        }
    }
    
    return count;
}

/// <summary>
/// Get current WiFi monitor status for debugging
/// </summary>
int WiFiMonitor_GetStatusString(const WiFiMonitorState* monitor, char* statusBuffer, size_t bufferSize) {
    if (!monitor || !statusBuffer || bufferSize == 0) {
        return -1;
    }
    
    const WiFiStatusRecord* status = &monitor->currentStatus;
    
    int result = snprintf(statusBuffer, bufferSize,
        "WiFi Monitor Status: %s | Signal: %d dBm | SSID: %s | Uptime: %.1fh | Reconnects: %d",
        status->isConnected ? "Connected" : "Disconnected",
        status->signalStrength,
        status->ssid,
        status->uptimeHours,
        monitor->reconnectCount
    );
    
    if (result < 0 || (size_t)result >= bufferSize) {
        return -1;
    }
    
    return 0;
}