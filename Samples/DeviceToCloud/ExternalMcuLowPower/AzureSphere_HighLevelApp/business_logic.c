/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <applibs/log.h>

#include "configuration.h"

#include "business_logic.h"
#include "color.h"
#include "cloud.h"
#include "eventloop_timer_utilities.h"
#include "exitcode.h"
#include "mcu_messaging.h"
#include "persistent_storage.h"
#include "power.h"
#include "status.h"
#include "telemetry.h"
#include "update.h"

static void Initialize(void);
static void CalculateAndSendTelemetry(void);
static void LogTelemetry(const DeviceTelemetry *const telemetry);

static void HandleMcuMessageFailure(void);
static void HandleInitResponseReceived(void);
static void HandleTelemetryResponseReceived(const DeviceTelemetry *telemetry);
static void HandleSetLedResponseReceived(const LedColor *color);

static void HandleCloudSendTelemetryAck(bool success);
static void HandleCloudFlavorAckReceived(bool success);

static void HandleTimeout(EventLoopTimer *timer);

// Application state
typedef enum {
    State_Initializing,
    State_WaitForMcu,
    State_WaitForCloud,
    State_GatherTelemetry,
    State_WaitForTelemetry,
    State_SendTelemetry,
    State_WaitForTelemetryAck,
    State_PersistTelemetry,
    State_WaitForFlavor,
    State_WaitForUpdate,
    State_TimedOut,
    State_WaitForUpdatesAfterTimeout,
    State_Sleep,
    State_Reboot,
    State_Success,
    State_Failure,
    State_Invalid = -1
} State;

static State applicationState = State_Invalid;
static bool mcuReady;
static bool cloudReady;
static bool haveTelemetry;
static DeviceTelemetry telemetry;
static bool telemetryReceivedByCloud;
static bool haveFlavor;
static char *receivedFlavorName;
static bool flavorAckByCloud;
static bool updateCheckComplete;
static bool rebootNeededForUpdates;

static ExitCode businessLogicExitCode;

static EventLoopTimer *timeoutTimer = NULL;

static const long timeoutPeriodInSeconds = 120;

ExitCode BusinessLogic_Initialize(EventLoop *el)
{
    applicationState = State_Initializing;
    mcuReady = false;
    cloudReady = false;
    haveTelemetry = false;
    telemetryReceivedByCloud = false;
    haveFlavor = false;
    receivedFlavorName = NULL;
    flavorAckByCloud = false;
    updateCheckComplete = false;
    rebootNeededForUpdates = false;
    businessLogicExitCode = ExitCode_Success;

    timeoutTimer = CreateEventLoopDisarmedTimer(el, HandleTimeout);
    if (timeoutTimer == NULL) {
        return ExitCode_BusinessLogic_TimeoutTimerCreate;
    }
    struct timespec ts = {.tv_sec = timeoutPeriodInSeconds, .tv_nsec = 0};

    int result = SetEventLoopTimerOneShot(timeoutTimer, &ts);
    if (result != 0) {
        return ExitCode_BusinessLogic_SetTimeoutTimer;
    }

    return ExitCode_Success;
}

bool BusinessLogic_Run(ExitCode *ec)
{
    bool finished;
    do {
        finished = true;
        switch (applicationState) {
        case State_Invalid:
            Log_Debug("ERROR: Invalid application state.\n");
            break;
        case State_Initializing:
            Status_NotifyStarting();
            Initialize();
            applicationState = State_WaitForMcu;
            break;
        case State_WaitForMcu:
            if (mcuReady) {
                applicationState = State_WaitForCloud;
                finished = false;
            }
            break;
        case State_WaitForCloud:
            if (cloudReady) {
                applicationState = State_GatherTelemetry;
                finished = false;
            }
            break;
        case State_GatherTelemetry:
            McuMessaging_RequestTelemetry(HandleTelemetryResponseReceived, HandleMcuMessageFailure);
            applicationState = State_WaitForTelemetry;
            break;
        case State_WaitForTelemetry:
            if (haveTelemetry) {
                applicationState = State_SendTelemetry;
                finished = false;
            }
            break;
        case State_SendTelemetry:
            CalculateAndSendTelemetry();
            applicationState = State_WaitForTelemetryAck;
            break;
        case State_WaitForTelemetryAck:
            if (telemetryReceivedByCloud) {
                applicationState = State_PersistTelemetry;
                finished = false;
            }
            break;
        case State_PersistTelemetry:
            PersistentStorage_PersistTelemetry(&telemetry);
            applicationState = State_WaitForFlavor;
            finished = false;
            break;
        case State_WaitForFlavor:
            if (haveFlavor && flavorAckByCloud) {
                applicationState = State_WaitForUpdate;
                Update_NotifyBusinessLogicComplete();
                DisarmEventLoopTimer(timeoutTimer);
                finished = false;
            }
            break;
        case State_WaitForUpdate:
            if (updateCheckComplete) {
                if (rebootNeededForUpdates) {
                    applicationState = State_Reboot;
                } else {
                    applicationState = State_Sleep;
                }
                finished = false;
            }
            break;
        case State_TimedOut:
            if (updateCheckComplete) {
                if (rebootNeededForUpdates) {
                    applicationState = State_Reboot;
                } else {
                    applicationState = State_Sleep;
                }
            } else {
                Log_Debug("INFO: Waiting for update check to complete after timeout\n");
                applicationState = State_WaitForUpdatesAfterTimeout;
            }
            finished = false;
            break;
        case State_WaitForUpdatesAfterTimeout:
            if (updateCheckComplete) {
                applicationState = State_TimedOut;
                finished = false;
            }
            break;
        case State_Reboot:
            Status_NotifyFinished();
            Log_Debug("INFO: Requesting device reboot.\n");
            Power_RequestReboot();
            applicationState =
                (businessLogicExitCode == ExitCode_Success) ? State_Success : State_Failure;
            finished = false;
            break;
        case State_Sleep:
            Status_NotifyFinished();
            Log_Debug("INFO: Requesting device power-down.\n");
            Power_RequestPowerdown();
            applicationState =
                (businessLogicExitCode == ExitCode_Success) ? State_Success : State_Failure;
            finished = false;
            break;
        case State_Success:
            Log_Debug("---------- COMPLETED SUCCESSFULLY ------");
            break;
        case State_Failure:
            break;
        }
    } while (!finished);

    bool logicComplete = applicationState == State_Success || applicationState == State_Failure;
    if (logicComplete) {
        *ec = businessLogicExitCode;
    }

    return logicComplete;
}

void BusinessLogic_NotifyCloudConnectionChange(bool connected)
{
    Log_Debug("INFO: Cloud connection: %s\n", connected ? "established" : "disconnected");
    cloudReady = connected;
}

void BusinessLogic_NotifyCloudFlavorChange(const LedColor *color, const char *flavorName)
{
    if (color != NULL) {
        Log_Debug("INFO: Sending SetLed RGB (%d, %d, %d)\n", color->red ? 1 : 0,
                  color->green ? 1 : 0, color->blue ? 1 : 0);
        McuMessaging_SetLed(color, HandleSetLedResponseReceived, HandleMcuMessageFailure);
        if (flavorName != NULL) {
            receivedFlavorName = strdup(flavorName);
        }
    } else {
        Log_Debug("INFO: No color change - sending flavor change acknowledgement.\n");
        Cloud_SendFlavorAcknowledgement(color, flavorName, HandleCloudFlavorAckReceived);
    }
}

void BusinessLogic_NotifyUpdateCheckComplete(bool rebootRequired)
{
    updateCheckComplete = true;
    rebootNeededForUpdates = rebootRequired;
    Log_Debug("INFO: Update complete - reboot %s.\n", rebootRequired ? "required" : "not required");
}

void BusinessLogic_NotifyUpdateCheckFailed(ExitCode exitCode)
{
    Log_Debug("ERROR: Update check failed (exit code %u)\n", exitCode);

    // Flag the update check as complete, but allow the business logic to continue.
    // Save the ExitCode to return on completion.
    updateCheckComplete = true;
    businessLogicExitCode = exitCode;
}

void BusinessLogic_NotifyFatalError(ExitCode exitCode)
{
    Log_Debug("ERROR: Fatal error in business logic (exit code %u)\n", exitCode);

    // At this point, the business logic is effectively terminated, so we skip forward to the
    // update check, and save the ExitCode to return on completion.

    applicationState = State_WaitForUpdate;
    businessLogicExitCode = exitCode;
}

static void Initialize(void)
{
    McuMessaging_Init(HandleInitResponseReceived, HandleMcuMessageFailure);
}

static void CalculateAndSendTelemetry()
{
    CloudTelemetry cloudTelemetry;
    DeviceTelemetry previousTelemetry;
    bool retrievedTelemetry = PersistentStorage_RetrieveTelemetry(&previousTelemetry);

    if (retrievedTelemetry) {
        Log_Debug("INFO: Previous telemetry found in persistent storage: \n");
        LogTelemetry(&previousTelemetry);
        cloudTelemetry.dispensesSinceLastSync =
            telemetry.lifetimeTotalDispenses - previousTelemetry.lifetimeTotalDispenses;
    } else {
        cloudTelemetry.dispensesSinceLastSync = telemetry.lifetimeTotalDispenses;
    }

    cloudTelemetry.lifetimeTotalDispenses = telemetry.lifetimeTotalDispenses;
    cloudTelemetry.remainingDispenses =
        telemetry.lifetimeTotalStockedDispenses - telemetry.lifetimeTotalDispenses;
    cloudTelemetry.lowSoda = cloudTelemetry.remainingDispenses <= LowDispenseAlertThreshold;
    cloudTelemetry.batteryLevel = telemetry.batteryLevel;

    Cloud_SendTelemetry(&cloudTelemetry, HandleCloudSendTelemetryAck);
}

static void HandleMcuMessageFailure(void)
{
    // We consider missing responses from the MCU to be fatal errors; a more sophisticated
    // implementation may retry here.
    Log_Debug("ERROR: Timed out waiting for MCU response.\n");
    BusinessLogic_NotifyFatalError(ExitCode_McuMessaging_Timeout);
}

static void HandleInitResponseReceived(void)
{
    Log_Debug("INFO: Init sent to MCU and response received.\n");
    mcuReady = true;
}

static void LogTelemetry(const DeviceTelemetry *const telemetry)
{
    Log_Debug("INFO: Total dispenses: %u\n", telemetry->lifetimeTotalDispenses);
    Log_Debug("INFO: Total stocked dispenses: %u\n", telemetry->lifetimeTotalStockedDispenses);
    Log_Debug("INFO: Capacity: %u\n", telemetry->capacity);
    Log_Debug("INFO: Battery level: %.2fV\n", telemetry->batteryLevel);
}

static void HandleTelemetryResponseReceived(const DeviceTelemetry *receivedTelemetry)
{
    Log_Debug("INFO: Telemetry received from MCU: \n");
    LogTelemetry(receivedTelemetry);

    telemetry = *receivedTelemetry;
    haveTelemetry = true;
}

static void HandleSetLedResponseReceived(const LedColor *color)
{
    Log_Debug("INFO: SetLed sent to device and response received: RGB (%d, %d, %d).\n",
              color->red ? 1 : 0, color->green ? 1 : 0, color->blue ? 1 : 0);
    haveFlavor = true;

    if (Cloud_SendFlavorAcknowledgement(color, receivedFlavorName, HandleCloudFlavorAckReceived)) {
        free(receivedFlavorName);
        receivedFlavorName = NULL;
    }
}

static void HandleCloudSendTelemetryAck(bool success)
{
    Log_Debug("INFO: Telemetry received by cloud\n");
    telemetryReceivedByCloud = success;
}

static void HandleCloudFlavorAckReceived(bool success)
{
    Log_Debug("INFO: Flavor ack received by cloud\n");
    flavorAckByCloud = success;
}

static void HandleTimeout(EventLoopTimer *timer)
{
    Log_Debug("ERROR: Timed out before business logic could complete.\n");

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: Could not consume timeout timer event\n");
    }

    applicationState = State_TimedOut;
}
