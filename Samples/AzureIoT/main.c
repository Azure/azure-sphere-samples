/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application demonstrates how to interface Azure Sphere devices with Azure IoT
// services. Using the Azure IoT SDK C APIs, it shows how to:
// 1. Use Device Provisioning Service (DPS) to connect to Azure IoT Hub/Central with
// certificate-based authentication
// 2. Use Device Twin to upload simulated temperature measurements, upload button press events and
// receive a desired LED state from Azure IoT Hub/Central
// 3. Use Direct Methods to receive a "Trigger Alarm" command from Azure IoT Hub/Central

// You will need to provide four pieces of information to use this application, all of which are set
// in the app_manifest.json.
// 1. The Scope Id for the Device Provisioning Service - DPS (set in 'CmdArgs')
// 2. The Tenant Id obtained from 'azsphere tenant show-selected' (set in 'DeviceAuthentication')
// 3. The Azure DPS Global endpoint address 'global.azure-devices-provisioning.net'
//    (set in 'AllowedConnections')
// 4. The Azure IoT Hub Endpoint address(es) that DPS is configured to direct this device to (set in
// 'AllowedConnections')

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/gpio.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>

// By default, this sample targets hardware that follows the MT3620 Reference
// Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. See
// https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

#include "eventloop_timer_utilities.h"

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Main_EventLoopFail = 2,

    ExitCode_ButtonTimer_Consume = 3,

    ExitCode_AzureTimer_Consume = 4,

    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_MessageButton = 6,
    ExitCode_Init_OrientationButton = 7,
    ExitCode_Init_TwinStatusLed = 8,
    ExitCode_Init_ButtonPollTimer = 9,
    ExitCode_Init_AzureTimer = 10,

    ExitCode_IsButtonPressed_GetValue = 11
} ExitCode;

static volatile sig_atomic_t exitCode = ExitCode_Success;

#include "parson.h" // used to parse Device Twin messages.

// Azure IoT defines.
static char *scopeId; // ScopeId for DPS
static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubAuthenticated = false;

// Function declarations
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context);
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback);
static void TwinReportState(const char *jsonState);
static void ReportedStateCallback(int result, void *context);
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback);
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);
static void SendTelemetry(const char *jsonMessage);
static void SetupAzureClient(void);
static void SendSimulatedTelemetry(void);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void AzureTimerEventHandler(EventLoopTimer *timer);

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
// Button
static int sendMessageButtonGpioFd = -1;

// LED
static int deviceTwinStatusLedGpioFd = -1;

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;
static EventLoopTimer *azureTimer = NULL;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 1;        // poll azure iot every second
static const int AzureIoTPollPeriodsPerTelemetry = 5;         // only send telemetry 1/5 of polls
static const int AzureIoTMinReconnectPeriodSeconds = 60;      // back off when reconnecting
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60; // back off limit

static int azureIoTPollPeriodSeconds = -1;
static int telemetryCount = 0;

// State variables
static GPIO_Value_Type sendMessageButtonState = GPIO_Value_High;
static bool statusLedOn = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Azure IoT Application starting.\n");

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    if (argc > 1) {
        scopeId = argv[1];
        Log_Debug("Using Azure IoT DPS Scope ID %s\n", scopeId);
    } else {
        Log_Debug("ScopeId needs to be set in the app_manifest CmdArgs\n");
        return -1;
    }

    exitCode = InitPeripheralsAndHandlers();

    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");

    return exitCode;
}

/// <summary>
/// Button timer event:  Check the status of the button
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    if (IsButtonPressed(sendMessageButtonGpioFd, &sendMessageButtonState)) {
        SendTelemetry("{\"ButtonPress\" : \"True\"}");
    }
}

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AzureTimer_Consume;
        return;
    }

    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady && !iothubAuthenticated) {
            SetupAzureClient();
        }
    } else {
        Log_Debug("Failed to get Network state\n");
    }

    if (iothubAuthenticated) {
        telemetryCount++;
        if (telemetryCount == AzureIoTPollPeriodsPerTelemetry) {
            telemetryCount = 0;
            SendSimulatedTelemetry();
        }
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    sendMessageButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (sendMessageButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_MessageButton;
    }

    // SAMPLE_LED is used to show Device Twin settings state
    Log_Debug("Opening SAMPLE_LED as output.\n");
    deviceTwinStatusLedGpioFd =
        GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (deviceTwinStatusLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_LED: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_TwinStatusLed;
    }

    // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL) {
        return ExitCode_Init_AzureTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
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
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(azureTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");

    // Leave the LEDs off
    if (deviceTwinStatusLedGpioFd >= 0) {
        GPIO_SetValue(deviceTwinStatusLedGpioFd, GPIO_Value_High);
    }

    CloseFdAndPrintError(sendMessageButtonGpioFd, "SendMessageButton");
    CloseFdAndPrintError(deviceTwinStatusLedGpioFd, "StatusLed");
}

/// <summary>
///     Callback when the Azure IoT connection state changes.
///     This can indicate that a new connection attempt has succeeded or failed.
///     It can also indicate than an existing connection has expired due to SAS token expiry.
/// </summary>
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                     IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                     void *userContextCallback)
{
    iothubAuthenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
    Log_Debug("Azure IoT connection status: %s\n", GetReasonString(reason));
    if (iothubAuthenticated) {
        // Send static device twin properties when connection is established
        TwinReportState(
            "{\"manufacturer\":\"Microsoft\",\"model\":\"Azure Sphere Sample Device\"}");
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetupAzureClient(void)
{
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
    }

    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
                                                                          &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
              GetAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {

        // If we fail to connect, reduce the polling frequency, starting at
        // AzureIoTMinReconnectPeriodSeconds and with a backoff up to
        // AzureIoTMaxReconnectPeriodSeconds
        if (azureIoTPollPeriodSeconds == AzureIoTDefaultPollPeriodSeconds) {
            azureIoTPollPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
        } else {
            azureIoTPollPeriodSeconds *= 2;
            if (azureIoTPollPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
                azureIoTPollPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
            }
        }

        struct timespec azureTelemetryPeriod = {azureIoTPollPeriodSeconds, 0};
        SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

        Log_Debug("ERROR: Failed to create IoTHub Handle - will retry in %i seconds.\n",
                  azureIoTPollPeriodSeconds);
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

    iothubAuthenticated = true;

    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
                                        &keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"%s\".\n",
                  OPTION_KEEP_ALIVE);
        return;
    }

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, DeviceTwinCallback, NULL);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, DeviceMethodCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle, ConnectionStatusCallback,
                                                      NULL);
}

/// <summary>
///     Callback invoked when a Direct Method is received from Azure IoT Hub.
/// </summary>
static int DeviceMethodCallback(const char *methodName, const unsigned char *payload,
                                size_t payloadSize, unsigned char **response, size_t *responseSize,
                                void *userContextCallback)
{
    int result;
    char *responseString;

    Log_Debug("Received Device Method callback: Method name %s.\n", methodName);

    if (strcmp("TriggerAlarm", methodName) == 0) {
        // Output alarm using Log_Debug
        Log_Debug("  ----- ALARM TRIGGERED! -----\n");
        responseString = "\"Alarm Triggered\""; // must be a JSON string (in quotes)
        result = 200;
    } else {
        // All other method names are ignored
        responseString = "{}";
        result = -1;
    }

    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
    *responseSize = strlen(responseString);
    *response = malloc(*responseSize);
    memcpy(*response, responseString, *responseSize);
    return result;
}

/// <summary>
///     Callback invoked when a Device Twin update is received from Azure IoT Hub.
/// </summary>
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                               size_t payloadSize, void *userContextCallback)
{
    size_t nullTerminatedJsonSize = payloadSize + 1;
    char *nullTerminatedJsonString = (char *)malloc(nullTerminatedJsonSize);
    if (nullTerminatedJsonString == NULL) {
        Log_Debug("ERROR: Could not allocate buffer for twin update payload.\n");
        abort();
    }

    // Copy the provided buffer to a null terminated buffer.
    memcpy(nullTerminatedJsonString, payload, payloadSize);
    // Add the null terminator at the end.
    nullTerminatedJsonString[nullTerminatedJsonSize - 1] = 0;

    JSON_Value *rootProperties = NULL;
    rootProperties = json_parse_string(nullTerminatedJsonString);
    if (rootProperties == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
        goto cleanup;
    }

    JSON_Object *rootObject = json_value_get_object(rootProperties);
    JSON_Object *desiredProperties = json_object_dotget_object(rootObject, "desired");
    if (desiredProperties == NULL) {
        desiredProperties = rootObject;
    }

    // The desired properties should have a "StatusLED" object
    JSON_Object *LEDState = json_object_dotget_object(desiredProperties, "StatusLED");
    if (LEDState != NULL) {
        // ... with a "value" field which is a Boolean
        int statusLedValue = json_object_get_boolean(LEDState, "value");
        if (statusLedValue != -1) {
            statusLedOn = statusLedValue == 1;
            GPIO_SetValue(deviceTwinStatusLedGpioFd,
                          statusLedOn ? GPIO_Value_Low : GPIO_Value_High);
        }
    }

    // Report current status LED state
    if (statusLedOn) {
        TwinReportState("{\"StatusLED\":{\"value\":true}}");
    } else {
        TwinReportState("{\"StatusLED\":{\"value\":false}}");
    }

cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
    free(nullTerminatedJsonString);
}

/// <summary>
///     Converts the Azure IoT Hub connection status reason to a string.
/// </summary>
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

/// <summary>
///     Sends telemetry to Azure IoT Hub
/// </summary>
static void SendTelemetry(const char *jsonMessage)
{
    Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", jsonMessage);

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Cannot send Azure IoT Hub telemetry because the network is not up.\n");
        return;
    }

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonMessage);

    if (messageHandle == 0) {
        Log_Debug("ERROR: unable to create a new IoTHubMessage.\n");
        return;
    }

    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendEventCallback,
                                             /*&callback_param*/ NULL) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure requesting IoTHubClient to send telemetry event.\n");
    } else {
        Log_Debug("INFO: IoTHubClient accepted the telemetry event for delivery.\n");
    }

    IoTHubMessage_Destroy(messageHandle);
}

/// <summary>
///     Callback invoked when the Azure IoT Hub send event request is processed.
/// </summary>
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub send telemetry event callback: status code %d.\n", result);
}

/// <summary>
///     Enqueues a report containing Device Twin reported properties. The report is not sent
///     immediately, but it is sent on the next invocation of IoTHubDeviceClient_LL_DoWork().
/// </summary>
static void TwinReportState(const char *jsonState)
{
    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: Azure IoT Hub client not initialized.\n");
    } else {
        if (IoTHubDeviceClient_LL_SendReportedState(
                iothubClientHandle, (const unsigned char *)jsonState, strlen(jsonState),
                ReportedStateCallback, NULL) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: Azure IoT Hub client error when reporting state '%s'.\n", jsonState);
        } else {
            Log_Debug("INFO: Azure IoT Hub client accepted request to report state '%s'.\n",
                      jsonState);
        }
    }
}

/// <summary>
///     Callback invoked when the Device Twin report state request is processed by Azure IoT Hub
///     client.
/// </summary>
static void ReportedStateCallback(int result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub Device Twin reported state callback: status code %d.\n", result);
}

#define TELEMETRY_BUFFER_SIZE 100

/// <summary>
///     Generate simulated telemetry and send to Azure IoT Hub.
/// </summary>
void SendSimulatedTelemetry(void)
{
    // generate a simulated temperature
    static float temperature = 50.0f;                    // starting temperature
    float delta = ((float)(rand() % 41)) / 20.0f - 1.0f; // between -1.0 and +1.0
    temperature += delta;

    char telemetryBuffer[TELEMETRY_BUFFER_SIZE];
    int len = snprintf(telemetryBuffer, TELEMETRY_BUFFER_SIZE, "{\"Temperature\":\"%3.2f\"}",
                       temperature);
    if (len < 0 || len >= TELEMETRY_BUFFER_SIZE) {
        Log_Debug("ERROR: Cannot write telemetry to buffer.\n");
        return;
    }
    SendTelemetry(telemetryBuffer);
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}
