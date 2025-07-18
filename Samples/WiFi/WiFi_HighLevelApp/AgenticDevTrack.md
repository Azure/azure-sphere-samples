# Azure Sphere WiFi Network Monitor - Development Plan (MQTT Architecture)

## Project Overview
Transform the existing Azure Sphere WiFi sample app into a comprehensive network monitoring solution with:
1. Periodic WiFi network status monitoring (configurable intervals: 5, 10, 15 minutes)
2. MQTT-based data publishing to broker for web dashboard consumption
3. Email notifications for network drops to administrators via MQTT/web app integration

## Architectural Shift: MQTT-Centric Design
**Inspiration from SampleDevCode.md**: Following your lab's proven MQTT patterns for IoT device communication, telemetry, and remote management.

## Updated Architecture Overview
```
Azure Sphere Device ? MQTT Broker ? Web Dashboard + Email Service
                 ?              ?
                   Config/Control Topics
```

## Development Plan (Revised)

### Phase 1: MQTT Client Integration (Week 1-2)

#### 1.1 MQTT Library Integration for Azure Sphere
```c
// Since Azure Sphere uses different libraries than Particle, we'll need:
#include <applibs/eventloop.h>
#include "mqtt_client.h"  // Azure Sphere MQTT client library

// Device Configuration (inspired by your Particle code)
typedef struct {
    const char* DEVICE_ID;     // "azure_sphere_wifi_monitor_01"
    const char* LOCATION;      // "lab", "office", etc.
    const char* FW_VERSION;    // "1.0.0"
    char mqttBrokerIP[16];     // "192.168.200.207"
    int mqttPort;              // 1883
    char mqttUsername[64];     // "leowise"
    char mqttPassword[64];     // "LM4F120!mqtt$"
} DeviceConfig;

// WiFi Status Structure (enhanced for MQTT)
typedef struct {
    unsigned long timestamp;
    bool isConnected;
    bool isNetworkReady;
    int signalStrength;        // RSSI
    char ssid[64];
    char errorMessage[256];
    int reconnectCount;        // Track reconnections like your Particle code
    float uptime;              // Hours since last reboot
} WiFiStatusRecord;
```

#### 1.2 Topic Structure (Following Your Pattern)
```c
// Topic helper function (adapted from your topicFor function)
String buildTopic(const char* category, const char* event) {
    return String::format("agentic/%s/%s/%s/%s", 
                         config.LOCATION, 
                         config.DEVICE_ID, 
                         category, 
                         event);
}

// Topic Examples:
// "agentic/lab/azure_sphere_wifi_monitor_01/wifi/status"
// "agentic/lab/azure_sphere_wifi_monitor_01/wifi/alert"
// "agentic/lab/azure_sphere_wifi_monitor_01/system/heartbeat"
// "agentic/lab/azure_sphere_wifi_monitor_01/system/config"
// "agentic/lab/azure_sphere_wifi_monitor_01/system/cmd"
```

#### 1.3 MQTT Connection Management (Based on Your Robust Reconnect Logic)
```c
typedef struct {
    std::queue<unsigned long> reconnectTimes;
    int reconnectCount;
    const int MAX_RECONNECTS = 10;
    unsigned long nextReconnectAttempt;
    int reconnectAttempts;
} MQTTConnectionState;

int MQTTReconnectClient(MQTTConnectionState* state);
void handleMQTTReconnects(MQTTConnectionState* state);
```

### Phase 2: WiFi Monitoring with MQTT Publishing (Week 2-3)

#### 2.1 Enhanced WiFi Status Monitoring
```c
typedef struct {
    WiFiStatusRecord currentStatus;
    WiFiStatusRecord history[5];  // Last 5 checks
    int historyIndex;
    bool hasDropped;
    unsigned long lastCheck;
    unsigned long checkInterval;  // 5, 10, or 15 minutes in milliseconds
} WiFiMonitorState;

// Main monitoring function
ExitCode performWiFiCheck(WiFiMonitorState* monitor) {
    WiFiStatusRecord record = {0};
    
    // Use existing Azure Sphere functions
    record.timestamp = time(NULL);
    record.isNetworkReady = (CheckNetworkReady() == ExitCode_Success);
    
    if (record.isNetworkReady) {
        WifiConfig_ConnectedNetwork connectedNetwork;
        if (WifiConfig_GetCurrentNetwork(&connectedNetwork) == 0) {
            record.isConnected = true;
            record.signalStrength = connectedNetwork.signalRssi;
            strncpy(record.ssid, (char*)connectedNetwork.ssid, connectedNetwork.ssidLength);
            record.ssid[connectedNetwork.ssidLength] = '\0';
        }
    }
    
    // Update monitor state
    monitor->history[monitor->historyIndex] = record;
    monitor->historyIndex = (monitor->historyIndex + 1) % 5;
    monitor->currentStatus = record;
    
    // Publish to MQTT
    publishWiFiStatus(&record);
    
    return ExitCode_Success;
}
```

#### 2.2 MQTT Publishing Functions (Following Your JSON Pattern)
```c
void publishWiFiStatus(const WiFiStatusRecord* record) {
    // Create JSON payload (similar to your motion sensor code)
    StaticJsonDocument<512> doc;
    doc["device_id"] = config.DEVICE_ID;
    doc["location"] = config.LOCATION;
    doc["type"] = "wifi_status";
    doc["timestamp"] = record->timestamp;
    doc["connected"] = record->isConnected;
    doc["network_ready"] = record->isNetworkReady;
    doc["signal_strength"] = record->signalStrength;
    doc["ssid"] = record->ssid;
    doc["fw_version"] = config.FW_VERSION;
    
    char payload[512];
    size_t len = serializeJson(doc, payload, sizeof(payload));
    payload[len] = '\0';
    
    String topic = buildTopic("wifi", "status");
    mqttClient.publish(topic.c_str(), payload);
    
    if (DEBUG) Log_Debug("WiFi Status Published: %s", payload);
}

void publishWiFiAlert(const char* alertType, const WiFiStatusRecord* record) {
    StaticJsonDocument<512> doc;
    doc["device_id"] = config.DEVICE_ID;
    doc["location"] = config.LOCATION;
    doc["type"] = "wifi_alert";
    doc["alert_type"] = alertType;  // "connection_lost", "signal_weak", "frequent_reconnects"
    doc["timestamp"] = record->timestamp;
    doc["signal_strength"] = record->signalStrength;
    doc["reconnect_count"] = record->reconnectCount;
    
    char payload[512];
    size_t len = serializeJson(doc, payload, sizeof(payload));
    payload[len] = '\0';
    
    String topic = buildTopic("wifi", "alert");
    mqttClient.publish(topic.c_str(), payload);
    
    if (DEBUG) Log_Debug("WiFi Alert Published: %s", payload);
}
```

#### 2.3 Heartbeat Implementation (Based on Your Pattern)
```c
void publishHeartbeat() {
    StaticJsonDocument<256> doc;
    doc["device_id"] = config.DEVICE_ID;
    doc["location"] = config.LOCATION;
    doc["status"] = "alive";
    doc["fw_version"] = config.FW_VERSION;
    doc["timestamp"] = time(NULL);
    doc["uptime"] = getUptimeHours();
    doc["free_memory"] = getFreeMemoryKB();
    
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    
    String topic = buildTopic("system", "heartbeat");
    mqttClient.publish(topic.c_str(), payload);
    
    if (DEBUG) Log_Debug("Heartbeat: %s", payload);
}
```

### Phase 3: Configuration Management via MQTT (Week 3)

#### 3.1 Remote Configuration (Following Your Config Update Pattern)
```c
// Subscribe to config topic on connection
void setupMQTTSubscriptions() {
    String configTopic = buildTopic("system", "config");
    String cmdTopic = buildTopic("system", "cmd");
    
    mqttClient.subscribe(configTopic.c_str());
    mqttClient.subscribe(cmdTopic.c_str());
}

// Handle config updates (adapted from your handleConfigUpdate)
void handleWiFiMonitorConfig(const char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        if (DEBUG) Log_Debug("Config JSON parse failed");
        return;
    }
    
    if (doc.containsKey("monitor_interval")) {
        int newInterval = doc["monitor_interval"].as<int>();
        if (newInterval == 5 || newInterval == 10 || newInterval == 15) {
            monitorConfig.intervalMinutes = newInterval;
            Log_Debug("Monitor interval updated to %d minutes", newInterval);
        }
    }
    
    if (doc.containsKey("debug")) {
        DEBUG = doc["debug"].as<bool>();
        Log_Debug("DEBUG flag updated: %s", DEBUG ? "true" : "false");
    }
    
    if (doc.containsKey("alert_threshold")) {
        alertConfig.signalThreshold = doc["alert_threshold"].as<int>();
    }
    
    // Save config to persistent storage
    saveConfigToStorage(&monitorConfig);
}

// Handle control commands
void handleWiFiMonitorCommands(const char* payload) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, payload);
    
    if (doc.containsKey("action")) {
        String action = doc["action"].as<String>();
        if (action == "force_check") {
            // Trigger immediate WiFi status check
            performWiFiCheck(&wifiMonitor);
        } else if (action == "restart_monitor") {
            // Restart monitoring service
            initializeWiFiMonitor(&wifiMonitor);
        } else if (action == "reboot") {
            // Graceful device reboot
            Log_Debug("Remote reboot requested");
            // Implement safe reboot procedure
        }
    }
}
```

### Phase 4: Web Dashboard Integration (Week 3-4)

#### 4.1 Web App MQTT Topics (For Your Separate Web Project)
```javascript
// Example web app MQTT subscription topics
const TOPICS = {
    status: 'agentic/+/+/wifi/status',        // All WiFi status updates
    alerts: 'agentic/+/+/wifi/alert',         // All WiFi alerts
    heartbeat: 'agentic/+/+/system/heartbeat' // Device heartbeats
};

// Web dashboard will display:
// - Real-time WiFi status for all monitored devices
// - Last 5 status checks per device
// - Alert dashboard with email trigger capabilities
// - Configuration interface for remote device management
```

#### 4.2 Data Structures for Web Dashboard
```json
// WiFi Status Message Format
{
    "device_id": "azure_sphere_wifi_monitor_01",
    "location": "lab",
    "type": "wifi_status",
    "timestamp": 1703123456,
    "connected": true,
    "network_ready": true,
    "signal_strength": -45,
    "ssid": "LabNetwork",
    "fw_version": "1.0.0"
}

// WiFi Alert Message Format
{
    "device_id": "azure_sphere_wifi_monitor_01",
    "location": "lab", 
    "type": "wifi_alert",
    "alert_type": "connection_lost",
    "timestamp": 1703123456,
    "signal_strength": -85,
    "reconnect_count": 3
}
```

### Phase 5: Email Integration via Web App (Week 4-5)

#### 5.1 Email Trigger Logic (In Web Application)
```javascript
// Web app listens for WiFi alerts and triggers emails
mqtt.on('message', (topic, message) => {
    if (topic.includes('/wifi/alert')) {
        const alert = JSON.parse(message.toString());
        
        // Email trigger conditions
        if (alert.alert_type === 'connection_lost') {
            sendEmailAlert('WiFi Connection Lost', alert);
        } else if (alert.alert_type === 'frequent_reconnects') {
            sendEmailAlert('Frequent WiFi Reconnects Detected', alert);
        }
    }
});
```

#### 5.2 Alert Conditions (In Azure Sphere Device)
```c
// Detect and publish various alert conditions
void checkForAlerts(WiFiMonitorState* monitor) {
    WiFiStatusRecord* current = &monitor->currentStatus;
    
    // Connection lost alert
    if (!current->isConnected && monitor->history[0].isConnected) {
        publishWiFiAlert("connection_lost", current);
    }
    
    // Weak signal alert
    if (current->signalStrength < -70 && current->isConnected) {
        publishWiFiAlert("signal_weak", current);
    }
    
    // Frequent reconnects (following your pattern)
    int recentReconnects = getReconnectsInWindow(3600); // Last hour
    if (recentReconnects > 3) {
        publishWiFiAlert("frequent_reconnects", current);
    }
}
```

### Phase 6: System Integration & Testing (Week 5-6)

#### 6.1 App Manifest Updates for MQTT
```json
{
    "SchemaVersion": 1,
    "Name": "WiFiMonitor_MQTTApp", 
    "ComponentId": "d2058930-996f-408f-bb75-4be9e0703e92",
    "EntryPoint": "/bin/app",
    "Capabilities": {
        "Gpio": ["$SAMPLE_BUTTON_1", "$SAMPLE_BUTTON_2"],
        "WifiConfig": true,
        "NetworkConfig": true,
        "AllowedConnections": [
            "192.168.200.207:1883"
        ],
        "SystemTime": true
    },
    "ApplicationType": "Default"
}
```

#### 6.2 Main Loop Integration (Following Your Pattern)
```c
void loop() {
    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastHeartbeat = 0;
    static int reconnectAttempts = 0;
    
    // MQTT connection management (like your code)
    if (!mqttClient.isConnected()) {
        handleMQTTReconnection(&mqttState);
    } else {
        // WiFi monitoring check
        if (millis() - lastWiFiCheck > monitorConfig.intervalMinutes * 60 * 1000) {
            performWiFiCheck(&wifiMonitor);
            checkForAlerts(&wifiMonitor);
            lastWiFiCheck = millis();
        }
        
        // Heartbeat (every 60 seconds like your code)
        if (millis() - lastHeartbeat > 60000) {
            publishHeartbeat();
            lastHeartbeat = millis();
        }
        
        mqttClient.loop();
    }
}
```

## Advantages of MQTT Architecture

### 1. **Scalability**
- Multiple Azure Sphere devices can publish to same broker
- Web dashboard can monitor entire fleet of WiFi monitors
- Easy to add new device types (temperature, motion, etc.)

### 2. **Proven Pattern**
- Follows your existing lab infrastructure
- Reuses MQTT broker and credentials
- Consistent topic structure across device types

### 3. **Remote Management**
- Configuration updates via MQTT (no physical access needed)
- Remote troubleshooting and control
- Centralized logging and monitoring

### 4. **Web Dashboard Benefits**
- Real-time updates for all devices
- Historical data visualization
- Centralized alert management
- Email integration at application level

### 5. **Flexibility**
- Easy to extend with additional sensors
- Dashboard can integrate multiple data sources
- Email rules can be sophisticated (rate limiting, escalation)

## Updated Timeline: 5-6 weeks total
- **Week 1-2**: MQTT client integration and basic publishing
- **Week 2-3**: WiFi monitoring and alert logic
- **Week 3**: Remote configuration via MQTT
- **Week 3-4**: Web dashboard development (separate project)
- **Week 4-5**: Email integration in web app
- **Week 5-6**: Integration testing and deployment

## Resource Requirements
- 1 Embedded Developer (Azure Sphere + MQTT)
- 1 Web Developer (Dashboard + Email integration)
- Existing MQTT infrastructure (already available)
- Email service integration costs: ~$10-20/month

This approach leverages your existing MQTT infrastructure and follows proven patterns from your Particle devices, making it much more maintainable and scalable!