/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"

// Maximum buffer sizes
#define MQTT_MAX_TOPIC_LENGTH 256
#define MQTT_MAX_PAYLOAD_LENGTH 1024
#define MQTT_MAX_CLIENT_ID_LENGTH 64
#define MQTT_MAX_USERNAME_LENGTH 64
#define MQTT_MAX_PASSWORD_LENGTH 64
#define MQTT_MAX_HOSTNAME_LENGTH 64

// Device Configuration for Agentic Controller compatibility
typedef struct {
    const char* DEVICE_ID;          // "azure_sphere_wifi_monitor_01"
    const char* LOCATION;           // "lab", "office", etc.
    const char* FW_VERSION;         // "1.0.0"
    char mqttBrokerIP[MQTT_MAX_HOSTNAME_LENGTH];    // "192.168.200.207"
    int mqttPort;                   // 1883
    char mqttUsername[MQTT_MAX_USERNAME_LENGTH];    // "leowise"
    char mqttPassword[MQTT_MAX_PASSWORD_LENGTH];    // "LM4F120!mqtt$"
    int monitorIntervalMinutes;     // 5, 10, or 15 minutes
    bool debugEnabled;              // Debug flag
    int signalThreshold;            // RSSI threshold for alerts
} DeviceConfig;

// WiFi Status Structure for MQTT publishing
typedef struct {
    time_t timestamp;
    bool isConnected;
    bool isNetworkReady;
    int signalStrength;             // RSSI in dBm
    char ssid[64];
    char errorMessage[256];
    int reconnectCount;             // Track reconnections
    float uptimeHours;              // Hours since last reboot
} WiFiStatusRecord;

// MQTT Connection State Management
typedef struct {
    bool isConnected;
    int reconnectCount;
    time_t lastReconnectTime;
    time_t nextReconnectAttempt;
    int reconnectAttempts;
    bool shouldReconnect;
} MQTTConnectionState;

// MQTT Client Structure
typedef struct {
    int socket_fd;
    MQTTConnectionState connectionState;
    DeviceConfig* config;
    EventLoop* eventLoop;
    EventLoopTimer* pingTimer;
    EventLoopTimer* reconnectTimer;
    char clientId[MQTT_MAX_CLIENT_ID_LENGTH];
    bool initialized;
} MQTTClient;

// Callback function types
typedef void (*MQTTMessageCallback)(const char* topic, const char* payload, size_t payloadLength);
typedef void (*MQTTConnectionCallback)(bool connected);

/// <summary>
/// Initialize the MQTT client with configuration
/// </summary>
/// <param name="client">MQTT client structure to initialize</param>
/// <param name="config">Device configuration</param>
/// <param name="eventLoop">Event loop for async operations</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Initialize(MQTTClient* client, DeviceConfig* config, EventLoop* eventLoop);

/// <summary>
/// Connect to the MQTT broker
/// </summary>
/// <param name="client">Initialized MQTT client</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Connect(MQTTClient* client);

/// <summary>
/// Disconnect from the MQTT broker
/// </summary>
/// <param name="client">Connected MQTT client</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Disconnect(MQTTClient* client);

/// <summary>
/// Publish a message to a topic
/// </summary>
/// <param name="client">Connected MQTT client</param>
/// <param name="topic">Topic to publish to</param>
/// <param name="payload">Message payload</param>
/// <param name="payloadLength">Length of payload</param>
/// <param name="qos">Quality of Service (0, 1, or 2)</param>
/// <param name="retain">Retain flag</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Publish(MQTTClient* client, const char* topic, const char* payload, 
                 size_t payloadLength, int qos, bool retain);

/// <summary>
/// Subscribe to a topic
/// </summary>
/// <param name="client">Connected MQTT client</param>
/// <param name="topic">Topic to subscribe to</param>
/// <param name="qos">Quality of Service (0, 1, or 2)</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Subscribe(MQTTClient* client, const char* topic, int qos);

/// <summary>
/// Unsubscribe from a topic
/// </summary>
/// <param name="client">Connected MQTT client</param>
/// <param name="topic">Topic to unsubscribe from</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Unsubscribe(MQTTClient* client, const char* topic);

/// <summary>
/// Check if client is connected
/// </summary>
/// <param name="client">MQTT client</param>
/// <returns>true if connected, false otherwise</returns>
bool MQTT_IsConnected(MQTTClient* client);

/// <summary>
/// Process MQTT events and maintain connection
/// </summary>
/// <param name="client">MQTT client</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_Loop(MQTTClient* client);

/// <summary>
/// Set message callback for incoming messages
/// </summary>
/// <param name="client">MQTT client</param>
/// <param name="callback">Callback function</param>
void MQTT_SetMessageCallback(MQTTClient* client, MQTTMessageCallback callback);

/// <summary>
/// Set connection status callback
/// </summary>
/// <param name="client">MQTT client</param>
/// <param name="callback">Callback function</param>
void MQTT_SetConnectionCallback(MQTTClient* client, MQTTConnectionCallback callback);

/// <summary>
/// Cleanup and destroy MQTT client
/// </summary>
/// <param name="client">MQTT client to cleanup</param>
void MQTT_Cleanup(MQTTClient* client);

/// <summary>
/// Build topic string following Agentic Controller pattern
/// Format: "agentic/{location}/{device_id}/{category}/{event}"
/// </summary>
/// <param name="config">Device configuration</param>
/// <param name="category">Topic category (e.g., "wifi", "system")</param>
/// <param name="event">Topic event (e.g., "status", "alert", "heartbeat")</param>
/// <param name="topicBuffer">Buffer to store the built topic</param>
/// <param name="bufferSize">Size of the topic buffer</param>
/// <returns>0 on success, -1 on failure</returns>
int MQTT_BuildTopic(const DeviceConfig* config, const char* category, const char* event,
                    char* topicBuffer, size_t bufferSize);

/// <summary>
/// Helper function to get current timestamp
/// </summary>
/// <returns>Current timestamp in seconds since epoch</returns>
time_t MQTT_GetTimestamp(void);

/// <summary>
/// Helper function to calculate uptime in hours
/// </summary>
/// <returns>Uptime in hours since system start</returns>
float MQTT_GetUptimeHours(void);