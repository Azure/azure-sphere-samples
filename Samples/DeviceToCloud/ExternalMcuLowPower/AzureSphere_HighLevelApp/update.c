/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>
#include <applibs/powermanagement.h>
#include <applibs/sysevent.h>

#include "eventloop_timer_utilities.h"
#include "exitcode.h"
#include "update.h"

static void WaitForUpdatesCheckTimerEventHandler(EventLoopTimer *timer);
static void WaitForUpdatesDownloadTimerEventHandler(EventLoopTimer *timer);
static void HandleUpdateEvent(SysEvent_Events event, SysEvent_Status status,
                              const SysEvent_Info *info, void *context);
static void NoUpdateAvailable(void);
static void FinishAndReboot(void);
static void UpdatesStarted(void);
static void UpdateReadyForInstall(SysEvent_Status status, const SysEvent_Info *info, void *context);
static const char *UpdateTypeToString(SysEvent_UpdateType updateType);

static bool businessLogicComplete = false;
static bool pendingUpdatesDeferred = false;

static Update_UpdatesCompleteCallback updateCompleteCallbackFunc = NULL;
static ExitCodeCallbackType exitCodeCallbackFunc = NULL;
static EventRegistration *updateEventRegistration = NULL;

// We allow two minutes for an update check
static EventLoopTimer *waitForUpdatesCheckTimer = NULL;
static const struct timespec waitForUpdatesCheckTimerInterval = {.tv_sec = 120, .tv_nsec = 0};

static EventLoopTimer *waitForUpdatesToDownloadTimer = NULL;
static const struct timespec waitForUpdatesToDownloadTimerInterval = {.tv_sec = 300, .tv_nsec = 0};

ExitCode Update_Initialize(EventLoop *el, Update_UpdatesCompleteCallback updateCompleteCallback,
                           ExitCodeCallbackType failureCallback)
{
    updateEventRegistration =
        SysEvent_RegisterForEventNotifications(el, SysEvent_Events_Mask, HandleUpdateEvent, NULL);
    if (updateEventRegistration == NULL) {
        Log_Debug("ERROR: Failed to register for no-update event notification: %s (%d)\n",
                  strerror(errno), errno);
        return ExitCode_Update_Init_NoUpdateEvent;
    }

    waitForUpdatesCheckTimer =
        CreateEventLoopDisarmedTimer(el, &WaitForUpdatesCheckTimerEventHandler);
    if (waitForUpdatesCheckTimer == NULL) {
        return ExitCode_Update_Init_CreateWaitForUpdatesCheckTimer;
    }
    int result =
        SetEventLoopTimerOneShot(waitForUpdatesCheckTimer, &waitForUpdatesCheckTimerInterval);
    if (result != 0) {
        return ExitCode_Update_Init_SetWaitForUpdatesCheckTimer;
    }

    waitForUpdatesToDownloadTimer =
        CreateEventLoopDisarmedTimer(el, &WaitForUpdatesDownloadTimerEventHandler);
    if (waitForUpdatesToDownloadTimer == NULL) {
        return ExitCode_Update_Init_CreateWaitForUpdatesDownloadTimer;
    }

    updateCompleteCallbackFunc = updateCompleteCallback;
    exitCodeCallbackFunc = failureCallback;
    businessLogicComplete = false;
    pendingUpdatesDeferred = false;

    return ExitCode_Success;
}

void Update_Cleanup(void)
{
    if (updateEventRegistration != NULL) {
        SysEvent_UnregisterForEventNotifications(updateEventRegistration);
    }

    DisposeEventLoopTimer(waitForUpdatesCheckTimer);
    DisposeEventLoopTimer(waitForUpdatesToDownloadTimer);
}

void Update_NotifyBusinessLogicComplete(void)
{
    businessLogicComplete = true;
}

static void WaitForUpdatesCheckTimerEventHandler(EventLoopTimer *timer)
{
    Log_Debug("WARNING: Timed out waiting for check for updates.\n");
    ConsumeEventLoopTimerEvent(timer);

    NoUpdateAvailable();
}

static void WaitForUpdatesDownloadTimerEventHandler(EventLoopTimer *timer)
{
    ConsumeEventLoopTimerEvent(timer);
    Log_Debug("WARNING: Timed out waiting for updates to download.\n");

    FinishAndReboot();
}

static void HandleUpdateEvent(SysEvent_Events event, SysEvent_Status status,
                              const SysEvent_Info *info, void *context)
{
    // If we've received an update event, we can disable the update check timer.
    if (DisarmEventLoopTimer(waitForUpdatesCheckTimer) == -1) {
        Log_Debug("ERROR: Failed to disarm update check timer: %s (%d)\n", strerror(errno), errno);
    }

    switch (event) {
    case SysEvent_Events_NoUpdateAvailable:
        NoUpdateAvailable();
        break;

    case SysEvent_Events_UpdateStarted:
        UpdatesStarted();
        break;

    case SysEvent_Events_UpdateReadyForInstall:
        UpdateReadyForInstall(status, info, context);
        break;

    default:
        Log_Debug("WARNING: Unexpected SysEvent '%u'\n", event);
        break;
    }
}

static void NoUpdateAvailable(void)
{
    if (updateCompleteCallbackFunc != NULL) {
        updateCompleteCallbackFunc(false);
    } else {
        Log_Debug("WARNING: No update complete callback handler registered\n");
    }
}

static void FinishAndReboot(void)
{
    if (updateCompleteCallbackFunc != NULL) {
        updateCompleteCallbackFunc(true);
    } else {
        Log_Debug(
            "ERROR: No update complete callback handler registered - unable to signal reboot\n");
    }
}

static void UpdatesStarted(void)
{
    int result = SetEventLoopTimerOneShot(waitForUpdatesToDownloadTimer,
                                          &waitForUpdatesToDownloadTimerInterval);
    if (result != 0) {
        Log_Debug("ERROR: Failed to start update download timer.\n");
        if (exitCodeCallbackFunc != NULL) {
            exitCodeCallbackFunc(ExitCode_Update_UpdatesStarted_SetWaitForUpdatesDownloadTimer);
        } else {
            Log_Debug("WARNING: No fatal error callback handler registered.\n");
        }
    }
}

static void UpdateReadyForInstall(SysEvent_Status status, const SysEvent_Info *info, void *context)
{
    SysEvent_Info_UpdateData data;
    int result = SysEvent_Info_GetUpdateData(info, &data);

    if (result == -1) {
        Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno), errno);
        if (exitCodeCallbackFunc != NULL) {
            exitCodeCallbackFunc(ExitCode_Update_UpdateCallback_GetUpdateData);
        } else {
            Log_Debug("WARNING: No fatal error callback handler registered.\n");
        }
        return;
    }

    Log_Debug("INFO: Update available - type: %s (%u).\n", UpdateTypeToString(data.update_type),
              data.update_type);

    switch (status) {
    case SysEvent_Status_Pending:
        if (businessLogicComplete) {
            Log_Debug("INFO: Allowing update.\n");
            SysEvent_ResumeEvent(SysEvent_Events_UpdateReadyForInstall);

        } else {
            Log_Debug("INFO: Max deferral time: %u minutes\n", data.max_deferral_time_in_minutes);
            Log_Debug("INFO: Deferring update for 1 minute.\n");
            result = SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, 1);

            if (result == -1) {
                Log_Debug("ERROR: SysEvent_DeferEvent: %s (%d).\n", strerror(errno), errno);
                if (exitCodeCallbackFunc != NULL) {
                    exitCodeCallbackFunc(ExitCode_Update_UpdateCallback_DeferEvent);
                } else {
                    Log_Debug("WARNING: No fatal error callback handler registered.\n");
                }
            }
        }
        break;

    case SysEvent_Status_Final:
        Log_Debug("INFO: Final update. App will update in 10 seconds.\n");
        FinishAndReboot();
        break;

    case SysEvent_Status_Deferred:
        Log_Debug("INFO: Update deferred.\n");
        break;

    case SysEvent_Status_Complete:
    default:
        Log_Debug("ERROR: Unexpected status %d.\n", status);
        if (exitCodeCallbackFunc != NULL) {
            exitCodeCallbackFunc(ExitCode_Update_UpdateCallback_UnexpectedStatus);
        } else {
            Log_Debug("WARNING: No fatal error callback handler registered.\n");
        }
        break;
    }
}

static const char *UpdateTypeToString(SysEvent_UpdateType updateType)
{
    switch (updateType) {
    case SysEvent_UpdateType_Invalid:
        return "Invalid";
    case SysEvent_UpdateType_App:
        return "Application";
    case SysEvent_UpdateType_System:
        return "System";
    default:
        return "Unknown";
    }
}
