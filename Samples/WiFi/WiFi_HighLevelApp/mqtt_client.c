/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "mqtt_client.h"
#include "eventloop_timer_utilities.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>

#include <applibs/log.h>
#include <applibs/networking.h>

// MQTT Protocol Constants
#define MQTT_PROTOCOL_VERSION 4
#define MQTT_CONNECT_FLAG_CLEAN_SESSION 0x02
#define MQTT_CONNECT_FLAG_WILL 0x04
#define MQTT_CONNECT_FLAG_WILL_QOS_1 0x08
#define MQTT_CONNECT_FLAG_WILL_QOS_2 0x10
#define MQTT_CONNECT_FLAG_WILL_RETAIN 0x20
#define MQTT_CONNECT_FLAG_PASSWORD 0x40
#define MQTT_CONNECT_FLAG_USERNAME 0x80

#define MQTT_MSG_TYPE_CONNECT 0x10
#define MQTT_MSG_TYPE_CONNACK 0x20
#define MQTT_MSG_TYPE_PUBLISH 0x30
#define MQTT_MSG_TYPE_PUBACK 0x40
#define MQTT_MSG_TYPE_SUBSCRIBE 0x82
#define MQTT_MSG_TYPE_SUBACK 0x90
#define MQTT_MSG_TYPE_UNSUBSCRIBE 0xA2
#define MQTT_MSG_TYPE_UNSUBACK 0xB0
#define MQTT_MSG_TYPE_PINGREQ 0xC0
#define MQTT_MSG_TYPE_PINGRESP 0xD0
#define MQTT_MSG_TYPE_DISCONNECT 0xE0

// Static variables for callbacks
static MQTTMessageCallback g_messageCallback = NULL;
static MQTTConnectionCallback g_connectionCallback = NULL;
static time_t g_systemStartTime = 0;

// Forward declarations for internal functions
static int SendMQTTPacket(MQTTClient* client, const uint8_t* packet, size_t length);
static int ReceiveMQTTPacket(MQTTClient* client, uint8_t* buffer, size_t bufferSize, size_t* receivedLength);
static int CreateConnectPacket(MQTTClient* client, uint8_t* buffer, size_t bufferSize, size_t* packetLength);
static int CreatePublishPacket(const char* topic, const char* payload, size_t payloadLength, 
                              int qos, bool retain, uint8_t* buffer, size_t bufferSize, size_t* packetLength);
static int CreateSubscribePacket(const char* topic, int qos, uint8_t* buffer, size_t bufferSize, size_t* packetLength);
static int ProcessIncomingMessage(MQTTClient* client, const uint8_t* packet, size_t length);
static void PingTimerHandler(EventLoopTimer* timer);
static void ReconnectTimerHandler(EventLoopTimer* timer);

/// <summary>
/// Initialize the MQTT client with configuration
/// </summary>
int MQTT_Initialize(MQTTClient* client, DeviceConfig* config, EventLoop* eventLoop) {
    if (!client || !config || !eventLoop) {
        Log_Debug("ERROR: Invalid parameters for MQTT_Initialize\n");
        return -1;
    }

    // Initialize system start time on first call
    if (g_systemStartTime == 0) {
        g_systemStartTime = time(NULL);
    }

    memset(client, 0, sizeof(MQTTClient));
    client->config = config;
    client->eventLoop = eventLoop;
    client->socket_fd = -1;
    
    // Generate unique client ID
    snprintf(client->clientId, sizeof(client->clientId), "azure_sphere_%lu", (unsigned long)time(NULL));
    #include <stdio.h> // Add this at the top of the file for snprintf
    #include <stdio.h> // Add this include at the top of the file for snprintf
    
    // Initialize connection state
    client->connectionState.isConnected = false;
    client->connectionState.reconnectCount = 0;
    client->connectionState.reconnectAttempts = 0;
    client->connectionState.shouldReconnect = true;
    
    // Create ping timer (60 second keepalive)
    static const struct timespec pingInterval = {.tv_sec = 60, .tv_nsec = 0};
    client->pingTimer = CreateEventLoopPeriodicTimer(eventLoop, PingTimerHandler, &pingInterval);
    if (!client->pingTimer) {
        Log_Debug("ERROR: Failed to create MQTT ping timer\n");
        return -1;
    }
    
    // Create reconnect timer (initially disarmed)
    client->reconnectTimer = CreateEventLoopDisarmedTimer(eventLoop, ReconnectTimerHandler);
    if (!client->reconnectTimer) {
        Log_Debug("ERROR: Failed to create MQTT reconnect timer\n");
        DisposeEventLoopTimer(client->pingTimer);
        return -1;
    }
    
    client->initialized = true;
    Log_Debug("INFO: MQTT client initialized for device %s at %s\n", 
              config->DEVICE_ID, config->LOCATION);
    
    return 0;
}

/// <summary>
/// Connect to the MQTT broker
/// </summary>
int MQTT_Connect(MQTTClient* client) {
    if (!client || !client->initialized) {
        Log_Debug("ERROR: MQTT client not initialized\n");
        return -1;
    }
    
    if (client->connectionState.isConnected) {
        return 0; // Already connected
    }
    
    // Create socket
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        Log_Debug("ERROR: Failed to create MQTT socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)client->config->mqttPort);
    
    if (inet_pton(AF_INET, client->config->mqttBrokerIP, &server_addr.sin_addr) <= 0) {
        Log_Debug("ERROR: Invalid MQTT broker IP address: %s\n", client->config->mqttBrokerIP);
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    // Connect to server
    if (connect(client->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Log_Debug("ERROR: Failed to connect to MQTT broker %s:%d - %s\n", 
                  client->config->mqttBrokerIP, client->config->mqttPort, strerror(errno));
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    // Send MQTT CONNECT packet
    uint8_t connectPacket[256];
    size_t packetLength;
    if (CreateConnectPacket(client, connectPacket, sizeof(connectPacket), &packetLength) != 0) {
        Log_Debug("ERROR: Failed to create MQTT CONNECT packet\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    if (SendMQTTPacket(client, connectPacket, packetLength) != 0) {
        Log_Debug("ERROR: Failed to send MQTT CONNECT packet\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    // Wait for CONNACK
    uint8_t response[4];
    size_t responseLength;
    if (ReceiveMQTTPacket(client, response, sizeof(response), &responseLength) != 0) {
        Log_Debug("ERROR: Failed to receive MQTT CONNACK\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    // Check CONNACK response
    if (responseLength >= 4 && response[0] == MQTT_MSG_TYPE_CONNACK && response[3] == 0) {
        client->connectionState.isConnected = true;
        client->connectionState.reconnectAttempts = 0;
        
        // Notify connection callback
        if (g_connectionCallback) {
            g_connectionCallback(true);
        }
        
        Log_Debug("INFO: MQTT connected to broker %s:%d\n", 
                  client->config->mqttBrokerIP, client->config->mqttPort);
        return 0;
    } else {
        Log_Debug("ERROR: MQTT CONNACK failed\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
}

/// <summary>
/// Publish a message to a topic
/// </summary>
int MQTT_Publish(MQTTClient* client, const char* topic, const char* payload, 
                 size_t payloadLength, int qos, bool retain) {
    if (!client || !client->connectionState.isConnected || !topic || !payload) {
        Log_Debug("ERROR: Invalid parameters or not connected for MQTT_Publish\n");
        return -1;
    }
    
    uint8_t publishPacket[MQTT_MAX_PAYLOAD_LENGTH + 256];
    size_t packetLength;
    
    if (CreatePublishPacket(topic, payload, payloadLength, qos, retain, 
                           publishPacket, sizeof(publishPacket), &packetLength) != 0) {
        Log_Debug("ERROR: Failed to create MQTT PUBLISH packet\n");
        return -1;
    }
    
    if (SendMQTTPacket(client, publishPacket, packetLength) != 0) {
        Log_Debug("ERROR: Failed to send MQTT PUBLISH packet\n");
        return -1;
    }
    
    if (client->config->debugEnabled) {
        Log_Debug("INFO: MQTT Published to %s: %.*s\n", topic, (int)payloadLength, payload);
    }
    
    return 0;
}

/// <summary>
/// Subscribe to a topic
/// </summary>
int MQTT_Subscribe(MQTTClient* client, const char* topic, int qos) {
    if (!client || !client->connectionState.isConnected || !topic) {
        Log_Debug("ERROR: Invalid parameters or not connected for MQTT_Subscribe\n");
        return -1;
    }
    
    uint8_t subscribePacket[MQTT_MAX_TOPIC_LENGTH + 32];
    size_t packetLength;
    
    if (CreateSubscribePacket(topic, qos, subscribePacket, sizeof(subscribePacket), &packetLength) != 0) {
        Log_Debug("ERROR: Failed to create MQTT SUBSCRIBE packet\n");
        return -1;
    }
    
    if (SendMQTTPacket(client, subscribePacket, packetLength) != 0) {
        Log_Debug("ERROR: Failed to send MQTT SUBSCRIBE packet\n");
        return -1;
    }
    
    Log_Debug("INFO: MQTT Subscribed to topic: %s\n", topic);
    return 0;
}

/// <summary>
/// Check if client is connected
/// </summary>
bool MQTT_IsConnected(MQTTClient* client) {
    return client && client->connectionState.isConnected;
}

/// <summary>
/// Process MQTT events and maintain connection
/// </summary>
int MQTT_Loop(MQTTClient* client) {
    if (!client || !client->initialized) {
        return -1;
    }
    
    if (!client->connectionState.isConnected) {
        return 0; // Not connected, nothing to process
    }
    
    // Check for incoming messages (non-blocking)
    uint8_t buffer[MQTT_MAX_PAYLOAD_LENGTH + 256];
    size_t receivedLength;
    
    // Set socket to non-blocking for peek
    int flags = fcntl(client->socket_fd, F_GETFL, 0);
    fcntl(client->socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    if (ReceiveMQTTPacket(client, buffer, sizeof(buffer), &receivedLength) == 0) {
        ProcessIncomingMessage(client, buffer, receivedLength);
    }
    
    // Restore blocking mode
    fcntl(client->socket_fd, F_SETFL, flags);
    
    return 0;
}

/// <summary>
/// Build topic string following Agentic Controller pattern
/// </summary>
int MQTT_BuildTopic(const DeviceConfig* config, const char* category, const char* event,
                    char* topicBuffer, size_t bufferSize) {
    if (!config || !category || !event || !topicBuffer || bufferSize == 0) {
        return -1;
    }
    
    int result = snprintf(topicBuffer, bufferSize, "agentic/%s/%s/%s/%s",
    #include <stdio.h> // Add this at the top of the file for snprintf
                         config->LOCATION, config->DEVICE_ID, category, event);
    
    if (result < 0 || (size_t)result >= bufferSize) {
        Log_Debug("ERROR: Topic buffer too small for: agentic/%s/%s/%s/%s\n",
                  config->LOCATION, config->DEVICE_ID, category, event);
        return -1;
    }
    
    return 0;
}

/// <summary>
/// Get current timestamp
/// </summary>
time_t MQTT_GetTimestamp(void) {
    return time(NULL);
}

/// <summary>
/// Get uptime in hours
/// </summary>
float MQTT_GetUptimeHours(void) {
    if (g_systemStartTime == 0) {
        return 0.0f;
    }
    
    time_t currentTime = time(NULL);
    return (float)(currentTime - g_systemStartTime) / 3600.0f;
}

/// <summary>
/// Set message callback
/// </summary>
void MQTT_SetMessageCallback(MQTTClient* client, MQTTMessageCallback callback) {
    (void)client; // Suppress unused parameter warning
    g_messageCallback = callback;
}

/// <summary>
/// Set connection callback
/// </summary>
void MQTT_SetConnectionCallback(MQTTClient* client, MQTTConnectionCallback callback) {
    (void)client; // Suppress unused parameter warning
    g_connectionCallback = callback;
}

/// <summary>
/// Cleanup MQTT client
/// </summary>
void MQTT_Cleanup(MQTTClient* client) {
    if (!client) {
        return;
    }
    
    if (client->connectionState.isConnected && client->socket_fd >= 0) {
        // Send DISCONNECT packet
        uint8_t disconnectPacket[2] = {MQTT_MSG_TYPE_DISCONNECT, 0};
        SendMQTTPacket(client, disconnectPacket, 2);
        close(client->socket_fd);
    }
    
    if (client->pingTimer) {
        DisposeEventLoopTimer(client->pingTimer);
    }
    
    if (client->reconnectTimer) {
        DisposeEventLoopTimer(client->reconnectTimer);
    }
    
    memset(client, 0, sizeof(MQTTClient));
    Log_Debug("INFO: MQTT client cleaned up\n");
}

// Internal helper functions implementation

static int SendMQTTPacket(MQTTClient* client, const uint8_t* packet, size_t length) {
    if (!client || client->socket_fd < 0 || !packet || length == 0) {
        return -1;
    }
    
    ssize_t sent = send(client->socket_fd, packet, length, 0);
    if (sent != (ssize_t)length) {
        Log_Debug("ERROR: Failed to send MQTT packet: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

static int ReceiveMQTTPacket(MQTTClient* client, uint8_t* buffer, size_t bufferSize, size_t* receivedLength) {
    if (!client || client->socket_fd < 0 || !buffer || !receivedLength) {
        return -1;
    }
    
    ssize_t received = recv(client->socket_fd, buffer, bufferSize, 0);
    if (received <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Log_Debug("ERROR: Failed to receive MQTT packet: %s\n", strerror(errno));
            // Connection lost, mark as disconnected
            client->connectionState.isConnected = false;
            if (g_connectionCallback) {
                g_connectionCallback(false);
            }
        }
        return -1;
    }
    
    *receivedLength = (size_t)received;
    return 0;
}

static int CreateConnectPacket(MQTTClient* client, uint8_t* buffer, size_t bufferSize, size_t* packetLength) {
    // This is a simplified MQTT CONNECT packet creation
    // In a production implementation, you'd want a more robust MQTT library
    
    size_t offset = 0;
    
    // Fixed header
    buffer[offset++] = MQTT_MSG_TYPE_CONNECT;
    
    // Calculate remaining length (simplified - assumes total packet < 127 bytes)
    size_t clientIdLen = strlen(client->clientId);
    size_t usernameLen = strlen(client->config->mqttUsername);
    size_t passwordLen = strlen(client->config->mqttPassword);
    size_t remainingLength = 10 + 2 + clientIdLen + 2 + usernameLen + 2 + passwordLen;
    
    buffer[offset++] = (uint8_t)remainingLength;
    
    // Variable header
    // Protocol name
    buffer[offset++] = 0x00; buffer[offset++] = 0x04; // Length
    buffer[offset++] = 'M'; buffer[offset++] = 'Q'; buffer[offset++] = 'T'; buffer[offset++] = 'T';
    
    // Protocol version
    buffer[offset++] = MQTT_PROTOCOL_VERSION;
    
    // Connect flags
    buffer[offset++] = MQTT_CONNECT_FLAG_CLEAN_SESSION | MQTT_CONNECT_FLAG_USERNAME | MQTT_CONNECT_FLAG_PASSWORD;
    
    // Keep alive (60 seconds)
    buffer[offset++] = 0x00; buffer[offset++] = 0x3C;
    
    // Payload
    // Client ID
    buffer[offset++] = (uint8_t)(clientIdLen >> 8);
    buffer[offset++] = (uint8_t)(clientIdLen & 0xFF);
    memcpy(&buffer[offset], client->clientId, clientIdLen);
    offset += clientIdLen;
    
    // Username
    buffer[offset++] = (uint8_t)(usernameLen >> 8);
    buffer[offset++] = (uint8_t)(usernameLen & 0xFF);
    memcpy(&buffer[offset], client->config->mqttUsername, usernameLen);
    offset += usernameLen;
    
    // Password
    buffer[offset++] = (uint8_t)(passwordLen >> 8);
    buffer[offset++] = (uint8_t)(passwordLen & 0xFF);
    memcpy(&buffer[offset], client->config->mqttPassword, passwordLen);
    offset += passwordLen;
    
    *packetLength = offset;
    return 0;
}

static int CreatePublishPacket(const char* topic, const char* payload, size_t payloadLength, 
                              int qos, bool retain, uint8_t* buffer, size_t bufferSize, size_t* packetLength) {
    // Simplified MQTT PUBLISH packet creation
    size_t offset = 0;
    size_t topicLen = strlen(topic);
    
    // Fixed header
    uint8_t flags = MQTT_MSG_TYPE_PUBLISH;
    if (retain) flags |= 0x01;
    if (qos == 1) flags |= 0x02;
    else if (qos == 2) flags |= 0x04;
    
    buffer[offset++] = flags;
    
    // Remaining length (simplified)
    size_t remainingLength = 2 + topicLen + payloadLength;
    buffer[offset++] = (uint8_t)remainingLength;
    
    // Variable header
    // Topic name
    buffer[offset++] = (uint8_t)(topicLen >> 8);
    buffer[offset++] = (uint8_t)(topicLen & 0xFF);
    memcpy(&buffer[offset], topic, topicLen);
    offset += topicLen;
    
    // Payload
    memcpy(&buffer[offset], payload, payloadLength);
    offset += payloadLength;
    
    *packetLength = offset;
    return 0;
}

static int CreateSubscribePacket(const char* topic, int qos, uint8_t* buffer, size_t bufferSize, size_t* packetLength) {
    // Simplified MQTT SUBSCRIBE packet creation
    size_t offset = 0;
    size_t topicLen = strlen(topic);
    
    // Fixed header
    buffer[offset++] = MQTT_MSG_TYPE_SUBSCRIBE;
    
    // Remaining length
    size_t remainingLength = 2 + 2 + topicLen + 1; // Packet ID + Topic + QoS
    buffer[offset++] = (uint8_t)remainingLength;
    
    // Variable header - Packet ID
    buffer[offset++] = 0x00; buffer[offset++] = 0x01;
    
    // Payload
    // Topic filter
    buffer[offset++] = (uint8_t)(topicLen >> 8);
    buffer[offset++] = (uint8_t)(topicLen & 0xFF);
    memcpy(&buffer[offset], topic, topicLen);
    offset += topicLen;
    
    // QoS
    buffer[offset++] = (uint8_t)qos;
    
    *packetLength = offset;
    return 0;
}

static int ProcessIncomingMessage(MQTTClient* client, const uint8_t* packet, size_t length) {
    if (length < 2) {
        return -1;
    }
    
    uint8_t messageType = packet[0] & 0xF0;
    
    switch (messageType) {
        case MQTT_MSG_TYPE_PUBLISH:
            // Extract topic and payload from PUBLISH message
            if (length < 4) break;
            
            size_t topicLen = (packet[2] << 8) | packet[3];
            if (length < 4 + topicLen) break;
            
            char topic[MQTT_MAX_TOPIC_LENGTH];
            if (topicLen >= sizeof(topic)) topicLen = sizeof(topic) - 1;
            memcpy(topic, &packet[4], topicLen);
            topic[topicLen] = '\0';
            
            const char* payloadStart = (const char*)&packet[4 + topicLen];
            size_t payloadLen = length - (4 + topicLen);
            
            if (g_messageCallback) {
                g_messageCallback(topic, payloadStart, payloadLen);
            }
            break;
            
        case MQTT_MSG_TYPE_PINGRESP:
            // Ping response received
            if (client->config->debugEnabled) {
                Log_Debug("INFO: MQTT PINGRESP received\n");
            }
            break;
            
        default:
            // Handle other message types as needed
            break;
    }
    
    return 0;
}

static void PingTimerHandler(EventLoopTimer* timer) {
    // This is a simplified approach - in practice, you'd pass the client through timer context
    // For now, we'll send a PINGREQ when this timer fires
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Failed to consume ping timer event\n");
        return;
    }
    
    // Note: In a full implementation, you'd need access to the client here
    // This could be done by storing a reference in the timer or using a global
    Log_Debug("INFO: MQTT ping timer fired\n");
}

static void ReconnectTimerHandler(EventLoopTimer* timer) {
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Failed to consume reconnect timer event\n");
        return;
    }
    
    // Note: In a full implementation, you'd attempt reconnection here
    Log_Debug("INFO: MQTT reconnect timer fired\n");
}