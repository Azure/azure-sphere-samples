/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// Azure Sphere WiFi Monitor with MQTT Integration for Agentic Controller
// This application monitors WiFi network status and publishes updates via MQTT
// for integration with the Agentic Controller system.

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/wificonfig.h>
#include <applibs/networking.h>
#include <applibs/log.h>

// Local includes for MQTT and WiFi monitoring
#include "mqtt_client.h"
#include "wifi_monitor.h"

// Hardware definition imports
#include <hw/sample_appliance.h>

// Event loop utilities
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_Init_EventLoop = 2,
    ExitCode_Init_Button = 3,
    ExitCode_Init_ButtonTimer = 4,
    ExitCode_Init_MQTT = 5,
    ExitCode_Init_WiFiMonitor = 6,
    ExitCode_Main_EventLoopFail = 7,
    ExitCode_ButtonTimer_Consume = 8,
    ExitCode_WiFiMonitorTimer_Consume = 9,
    ExitCode_HeartbeatTimer_Consume = 10
} ExitCode;

// Device Configuration - UPDATE THESE FOR YOUR ENVIRONMENT
static DeviceConfig deviceConfig = {
    .DEVICE_ID = "azure_sphere_wifi_monitor_01",
    .LOCATION = "lab",
    .FW_VERSION = "1.0.0",
    .mqttBrokerIP = "192.168.200.207",    // Update with your MQTT broker IP
    .mqttPort = 1883,
    .mqttUsername = "leowise",             // Update with your MQTT username
    .mqttPassword = "LM4F120!mqtt$",       // Update with your MQTT password
    .monitorIntervalMinutes = 5,           // Default to 5-minute intervals
    .debugEnabled = true,                  // Enable debug logging
    .signalThreshold = -70                 // Alert if signal weaker than -70 dBm
};

// WiFi Monitor Configuration
static WiFiMonitorConfig wifiMonitorConfig = {
    .intervalMinutes = 5,
    .signalThreshold = -70,
    .reconnectThreshold = 5,               // Alert after 5 reconnects per hour
    .alertOnDisconnect = true,
    .alertOnWeakSignal = true,
    .alertOnFrequentReconnects = true
};

// Global state variables
static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;
static EventLoopTimer *wifiMonitorTimer = NULL;
static EventLoopTimer *heartbeatTimer = NULL;
static int buttonGpioFd = -1;
static GPIO_Value_Type buttonState = GPIO_Value_High;
static volatile sig_atomic_t exitCode = ExitCode_Success;

static MQTTClient mqttClient = {0};
static WiFiMonitorState wifiMonitor = {0};

// Function declarations
static void TerminationHandler(int signalNumber);
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);
static void ButtonTimerHandler(EventLoopTimer *timer);
static void WiFiMonitorTimerHandler(EventLoopTimer *timer);
static void HeartbeatTimerHandler(EventLoopTimer *timer);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void OnMQTTMessage(const char* topic, const char* payload, size_t payloadLength);
static void OnMQTTConnection(bool connected);
static int PublishWiFiStatus(void);
static int PublishWiFiAlert(const char* alertType);
static int PublishHeartbeat(void);
static int SetupMQTTSubscriptions(void);
static void CloseFdAndPrintError(int fd, const char *fdName);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Azure Sphere WiFi Monitor starting...\n");
    Log_Debug("Device: %s | Location: %s | FW: %s\n", 
              deviceConfig.DEVICE_ID, deviceConfig.LOCATION, deviceConfig.FW_VERSION);
    Log_Debug("MQTT Broker: %s:%d | Interval: %d minutes\n",
              deviceConfig.mqttBrokerIP, deviceConfig.mqttPort, deviceConfig.monitorIntervalMinutes);

    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting with code %d.\n", exitCode);
    return exitCode;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    // Create event loop
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Initialize button for manual status check
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    buttonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }

    // Create button poll timer (check every 100ms)
    static const struct timespec buttonPollInterval = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerHandler, &buttonPollInterval);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonTimer;
    }

    // Initialize MQTT client
    if (MQTT_Initialize(&mqttClient, &deviceConfig, eventLoop) != 0) {
        Log_Debug("ERROR: Failed to initialize MQTT client\n");
        return ExitCode_Init_MQTT;
    }

    // Set MQTT callbacks
    MQTT_SetMessageCallback(&mqttClient, OnMQTTMessage);
    MQTT_SetConnectionCallback(&mqttClient, OnMQTTConnection);

    // Initialize WiFi Monitor
    if (WiFiMonitor_Initialize(&wifiMonitor, &wifiMonitorConfig) != 0) {
        Log_Debug("ERROR: Failed to initialize WiFi monitor\n");
        return ExitCode_Init_WiFiMonitor;
    }

    // Create WiFi monitoring timer (configurable interval)
    struct timespec wifiMonitorInterval = {
        .tv_sec = deviceConfig.monitorIntervalMinutes * 60,
        .tv_nsec = 0
    };
    wifiMonitorTimer = CreateEventLoopPeriodicTimer(eventLoop, &WiFiMonitorTimerHandler, &wifiMonitorInterval);
    if (wifiMonitorTimer == NULL) {
        return ExitCode_Init_WiFiMonitor;
    }

    // Create heartbeat timer (every 60 seconds)
    static const struct timespec heartbeatInterval = {.tv_sec = 60, .tv_nsec = 0};
    heartbeatTimer = CreateEventLoopPeriodicTimer(eventLoop, &HeartbeatTimerHandler, &heartbeatInterval);
    if (heartbeatTimer == NULL) {
        return ExitCode_Init_WiFiMonitor;
    }

    // Connect to MQTT broker
    Log_Debug("Connecting to MQTT broker...\n");
    if (MQTT_Connect(&mqttClient) != 0) {
        Log_Debug("WARNING: Initial MQTT connection failed, will retry automatically\n");
    } else {
        SetupMQTTSubscriptions();
    }

    // Perform initial WiFi check
    WiFiMonitor_PerformCheck(&wifiMonitor);
    
    Log_Debug("Initialization complete. Monitoring WiFi every %d minutes.\n", 
              deviceConfig.monitorIntervalMinutes);

    return ExitCode_Success;
}

/// <summary>
/// Button timer event: Check if button was pressed for manual WiFi check
/// </summary>
static void ButtonTimerHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check if button was pressed
    if (IsButtonPressed(buttonGpioFd, &buttonState)) {
        Log_Debug("Manual WiFi check triggered by button press\n");
        
        // Perform immediate WiFi check
        WiFiMonitor_PerformCheck(&wifiMonitor);
        
        // Publish status if MQTT is connected
        if (MQTT_IsConnected(&mqttClient)) {
            PublishWiFiStatus();
        }
        
        // Show current status
        char statusBuffer[256];
        if (WiFiMonitor_GetStatusString(&wifiMonitor, statusBuffer, sizeof(statusBuffer)) == 0) {
            Log_Debug("Current Status: %s\n", statusBuffer);
        }
    }
}

/// <summary>
/// WiFi monitor timer event: Perform periodic WiFi status check
/// </summary>
static void WiFiMonitorTimerHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_WiFiMonitorTimer_Consume;
        return;
    }

    Log_Debug("Performing scheduled WiFi status check...\n");
    
    // Perform WiFi status check
    if (WiFiMonitor_PerformCheck(&wifiMonitor) == 0) {
        // Check for alerts
        char alertType[64];
        if (WiFiMonitor_CheckAlerts(&wifiMonitor, alertType, sizeof(alertType))) {
            Log_Debug("WiFi Alert detected: %s\n", alertType);
            if (MQTT_IsConnected(&mqttClient)) {
                PublishWiFiAlert(alertType);
            }
        }
        
        // Publish status update
        if (MQTT_IsConnected(&mqttClient)) {
            PublishWiFiStatus();
        }
    }
    
    // Handle MQTT connection if needed
    if (!MQTT_IsConnected(&mqttClient)) {
        Log_Debug("MQTT not connected, attempting reconnection...\n");
        if (MQTT_Connect(&mqttClient) == 0) {
            SetupMQTTSubscriptions();
        }
    }
}

/// <summary>
/// Heartbeat timer event: Send periodic heartbeat message
/// </summary>
static void HeartbeatTimerHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_HeartbeatTimer_Consume;
        return;
    }

    if (MQTT_IsConnected(&mqttClient)) {
        PublishHeartbeat();
    }
    
    // Process MQTT events
    MQTT_Loop(&mqttClient);
}

/// <summary>
/// Check whether button has been pressed
/// </summary>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
    } else {
        // Button is pressed if it is low and different than last known state
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
/// MQTT message callback handler
/// </summary>
static void OnMQTTMessage(const char* topic, const char* payload, size_t payloadLength)
{
    if (!topic || !payload) {
        return;
    }
    
    // Create null-terminated payload string
    char payloadStr[payloadLength + 1];
    memcpy(payloadStr, payload, payloadLength);
    payloadStr[payloadLength] = '\0';
    
    if (deviceConfig.debugEnabled) {
        Log_Debug("MQTT Message received on %s: %s\n", topic, payloadStr);
    }
    
    // Build expected config topic
    char configTopic[MQTT_MAX_TOPIC_LENGTH];
    if (MQTT_BuildTopic(&deviceConfig, "system", "config", configTopic, sizeof(configTopic)) == 0) {
        if (strcmp(topic, configTopic) == 0) {
            WiFiMonitor_HandleConfigUpdate(&wifiMonitor, payloadStr);
            return;
        }
    }
    
    // Build expected command topic
    char cmdTopic[MQTT_MAX_TOPIC_LENGTH];
    if (MQTT_BuildTopic(&deviceConfig, "system", "cmd", cmdTopic, sizeof(cmdTopic)) == 0) {
        if (strcmp(topic, cmdTopic) == 0) {
            WiFiMonitor_HandleCommand(&wifiMonitor, payloadStr);
            return;
        }
    }
    
    Log_Debug("Received message on unknown topic: %s\n", topic);
}

/// <summary>
/// MQTT connection status callback
/// </summary>
static void OnMQTTConnection(bool connected)
{
    if (connected) {
        Log_Debug("MQTT connected successfully\n");
        SetupMQTTSubscriptions();
        
        // Publish initial status
        PublishWiFiStatus();
        PublishHeartbeat();
    } else {
        Log_Debug("MQTT disconnected\n");
    }
}

/// <summary>
/// Publish WiFi status to MQTT
/// </summary>
static int PublishWiFiStatus(void)
{
    char statusJSON[MQTT_MAX_PAYLOAD_LENGTH];
    char statusTopic[MQTT_MAX_TOPIC_LENGTH];
    
    if (WiFiMonitor_GetStatusJSON(&wifiMonitor, &deviceConfig, statusJSON, sizeof(statusJSON)) != 0) {
        Log_Debug("ERROR: Failed to create WiFi status JSON\n");
        return -1;
    }
    
    if (MQTT_BuildTopic(&deviceConfig, "wifi", "status", statusTopic, sizeof(statusTopic)) != 0) {
        Log_Debug("ERROR: Failed to build WiFi status topic\n");
        return -1;
    }
    
    if (MQTT_Publish(&mqttClient, statusTopic, statusJSON, strlen(statusJSON), 0, false) != 0) {
        Log_Debug("ERROR: Failed to publish WiFi status\n");
        return -1;
    }
    
    if (deviceConfig.debugEnabled) {
        Log_Debug("Published WiFi status: %s\n", statusJSON);
    }
    
    return 0;
}

/// <summary>
/// Publish WiFi alert to MQTT
/// </summary>
static int PublishWiFiAlert(const char* alertType)
{
    if (!alertType) {
        return -1;
    }
    
    char alertJSON[MQTT_MAX_PAYLOAD_LENGTH];
    char alertTopic[MQTT_MAX_TOPIC_LENGTH];
    
    if (WiFiMonitor_GetAlertJSON(&wifiMonitor, &deviceConfig, alertType, alertJSON, sizeof(alertJSON)) != 0) {
        Log_Debug("ERROR: Failed to create WiFi alert JSON\n");
        return -1;
    }
    
    if (MQTT_BuildTopic(&deviceConfig, "wifi", "alert", alertTopic, sizeof(alertTopic)) != 0) {
        Log_Debug("ERROR: Failed to build WiFi alert topic\n");
        return -1;
    }
    
    if (MQTT_Publish(&mqttClient, alertTopic, alertJSON, strlen(alertJSON), 0, false) != 0) {
        Log_Debug("ERROR: Failed to publish WiFi alert\n");
        return -1;
    }
    
    Log_Debug("Published WiFi alert: %s\n", alertJSON);
    return 0;
}

/// <summary>
/// Publish heartbeat to MQTT
/// </summary>
static int PublishHeartbeat(void)
{
    char heartbeatJSON[MQTT_MAX_PAYLOAD_LENGTH];
    char heartbeatTopic[MQTT_MAX_TOPIC_LENGTH];
    
    if (WiFiMonitor_GetHeartbeatJSON(&deviceConfig, heartbeatJSON, sizeof(heartbeatJSON)) != 0) {
        Log_Debug("ERROR: Failed to create heartbeat JSON\n");
        return -1;
    }
    
    if (MQTT_BuildTopic(&deviceConfig, "system", "heartbeat", heartbeatTopic, sizeof(heartbeatTopic)) != 0) {
        Log_Debug("ERROR: Failed to build heartbeat topic\n");
        return -1;
    }
    
    if (MQTT_Publish(&mqttClient, heartbeatTopic, heartbeatJSON, strlen(heartbeatJSON), 0, false) != 0) {
        Log_Debug("ERROR: Failed to publish heartbeat\n");
        return -1;
    }
    
    if (deviceConfig.debugEnabled) {
        Log_Debug("Published heartbeat: %s\n", heartbeatJSON);
    }
    
    return 0;
}

/// <summary>
/// Setup MQTT subscriptions for configuration and commands
/// </summary>
static int SetupMQTTSubscriptions(void)
{
    char configTopic[MQTT_MAX_TOPIC_LENGTH];
    char cmdTopic[MQTT_MAX_TOPIC_LENGTH];
    
    // Subscribe to config topic
    if (MQTT_BuildTopic(&deviceConfig, "system", "config", configTopic, sizeof(configTopic)) == 0) {
        if (MQTT_Subscribe(&mqttClient, configTopic, 0) == 0) {
            Log_Debug("Subscribed to config topic: %s\n", configTopic);
        }
    }
    
    // Subscribe to command topic
    if (MQTT_BuildTopic(&deviceConfig, "system", "cmd", cmdTopic, sizeof(cmdTopic)) == 0) {
        if (MQTT_Subscribe(&mqttClient, cmdTopic, 0) == 0) {
            Log_Debug("Subscribed to command topic: %s\n", cmdTopic);
        }
    }
    
    return 0;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    // Cleanup MQTT
    MQTT_Cleanup(&mqttClient);
    
    // Dispose timers
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(wifiMonitorTimer);
    DisposeEventLoopTimer(heartbeatTimer);
    
    // Close event loop
    EventLoop_Close(eventLoop);

    // Close file descriptors
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(buttonGpioFd, "ButtonGpio");
}