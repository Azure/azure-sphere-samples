/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates an application receiving
// notifications for a pending application update, and then deferring that update.
// On the MT3620 RDB,
// LED 2 is green when the update should be deferred, and yellow when it should be applied.
// Press button A to toggle between these modes.
// LED 3 is lit up blue when an OTA update is available.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button, digital output for LED)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - sysevent (receive notification of, defer, and accept pending application update)

#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/timerfd.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/eventloop.h>
#include <applibs/sysevent.h>

// By default, this sample's CMake build targets hardware that follows the MT3620
// Reference Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studios.
//
// To target different hardware, you'll need to update the CMake build. The necessary
// steps to do this vary depending on if you are building in Visual Studio, in Visual
// Studio Code or via the command line.
//
// See https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

#include "epoll_timerfd_utilities.h"

static volatile sig_atomic_t terminationRequired = false;

static int epollFd = -1;

// The accept mode LED triplet shows whether updates are allowed (yellow) or deferred (green).
static int acceptLedRedFd = -1;
static int acceptLedGreenFd = -1;
static int acceptLedBlueFd = -1;

// Press the button to toggle between accept or defer updates.
static int timerFd = -1;
static int buttonFd = -1;
static bool acceptUpdate = false;

static void UpdateAcceptModeLed(void);
static void SwitchOffAcceptModeLed(void);
static void ButtonTimerEventHandler(EventData *eventData);

// The pending update LED lights up the application is notified of a pending update.
static int pendingUpdateLedFd = -1;

static void SwitchOffPendingStatusLed(void);
static void UpdatePendingStatusLed(void);

// Application update events are received via an event loop.
static EventLoop *eventLoop = NULL;
static EventRegistration *updateEventReg = NULL;
static int eventLoopFd = -1;
static bool pendingUpdate = false;

static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context);
static const char *EventStatusToString(SysEvent_Status status);
static const char *UpdateTypeToString(SysEvent_UpdateType updateType);

static void TerminationHandler(int signalNumber);

static int SetUpSysEventHandler(void);
static void FreeSysEventHandler(void);

static int InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    terminationRequired = true;
}

/// <summary>
///     Set the accept mode LED to yellow if updates will be accepted,
///     and green if they should be deferred.
/// </summary>
static void UpdateAcceptModeLed(void)
{
    bool red, green, blue;

    // If updates are allowed, LED2 will be yellow (red + green)...
    if (acceptUpdate) {
        red = true;
        green = true;
        blue = false;
    }
    // ...else if updates should be deferred, LED will be green.
    else {
        red = false;
        green = true;
        blue = false;
    }

    GPIO_SetValue(acceptLedRedFd, red ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(acceptLedGreenFd, green ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(acceptLedBlueFd, blue ? GPIO_Value_Low : GPIO_Value_High);
}

/// <summary>
///     Switch off RGB components of accept mode LED.
/// </summary>
static void SwitchOffAcceptModeLed(void)
{
    if (acceptLedRedFd != -1) {
        GPIO_SetValue(acceptLedRedFd, GPIO_Value_High);
    }

    if (acceptLedGreenFd != -1) {
        GPIO_SetValue(acceptLedGreenFd, GPIO_Value_High);
    }

    if (acceptLedBlueFd != -1) {
        GPIO_SetValue(acceptLedBlueFd, GPIO_Value_High);
    }
}

/// <summary>
///     Handle button timer event by toggling the accept mode.
/// </summary>
/// <param name="eventData">EPOLL utilities cookie. Not used.</param>
static void ButtonTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(timerFd) != 0) {
        terminationRequired = true;
        return;
    }

    static GPIO_Value_Type buttonState = GPIO_Value_High;

    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(buttonFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        terminationRequired = true;
        return;
    }

    // If the button has just been pressed, then change update mode from
    // updates accepted to updates deferred or vice-versa.
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {
            acceptUpdate = !acceptUpdate;
            UpdateAcceptModeLed();
        }

        buttonState = newButtonState;
    }

    // If user has accepted updates and there is already a pending update then]
    // apply it immediately.
    if (acceptUpdate && pendingUpdate) {
        SysEvent_ResumeEvent(SysEvent_Events_Update);
    }
}

/// <summary>
///     Update the pending LED. If an update is available, the LED is switched
///     on. Otherwise, it is swtiched off.
/// </summary>
static void UpdatePendingStatusLed(void)
{
    GPIO_SetValue(pendingUpdateLedFd, pendingUpdate ? GPIO_Value_Low : GPIO_Value_High);
}

/// <summary>
///     Switch off the application update pending LED status.
/// </summary>
static void SwitchOffPendingStatusLed(void)
{
    if (pendingUpdateLedFd != -1) {
        GPIO_SetValue(pendingUpdateLedFd, GPIO_Value_High);
    }
}

/// <summary>
///     Called when a system event occurs.  This calls <see cref="EventLoop_Run" />
///     which invokes the specific event handler.
/// </summary>
/// <param name="eventData">EPOLL utilities cookie. Not used.</param>
static void SysEventHandler(EventData *eventData)
{
    if (EventLoop_Run(eventLoop, 0, true) == EventLoop_Run_Failed) {
        terminationRequired = true;
    }
}

/// <summary>
///     This function matches the SysEvent_EventsCallback signature, and is invoked
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context)
{
    if (event != SysEvent_Events_Update) {
        Log_Debug("ERROR: unexpected event: 0x%x\n", event);
        terminationRequired = true;
        return;
    }

    // Print GMT time at which message was received.
    char timeBuf[64];
    time_t t;
    time(&t);
    struct tm *tm = gmtime(&t);
    if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T", tm) != 0) {
        Log_Debug("INFO: Received update event: %s\n", timeBuf);
    }

    // Print information about received message.
    Log_Debug("INFO: Status: %s (%u)\n", EventStatusToString(status), status);

    SysEvent_Info_UpdateData data;
    int result = SysEvent_Info_GetUpdateData(info, &data);

    if (result == -1) {
        Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno), errno);
        terminationRequired = true;
        return;
    }

    Log_Debug("INFO: Max deferral time: %u minutes\n", data.max_deferral_time_in_minutes);
    Log_Debug("INFO: Update Type: %s (%u).\n", UpdateTypeToString(data.update_type),
              data.update_type);

    switch (status) {
        // If an update is pending, and the user has not allowed updates, then defer the update.
    case SysEvent_Status_Pending:
        pendingUpdate = true;
        if (acceptUpdate) {
            Log_Debug("INFO: Allowing update.\n");
        } else {
            Log_Debug("INFO: Deferring update for 1 minute.\n");
            result = SysEvent_DeferEvent(SysEvent_Events_Update, 1);
        }

        if (result == -1) {
            Log_Debug("ERROR: SysEvent_DeferEvent: %s (%d).\n", strerror(errno), errno);
            terminationRequired = true;
        }
        break;

    case SysEvent_Status_Final:
        Log_Debug("INFO: Final update. App will update in 10 seconds.\n");
        // Terminate app before it is forcibly shut down and replaced.
        // The application may be restarted before the update is applied.
        terminationRequired = true;
        break;

    case SysEvent_Status_Rejected:
        Log_Debug("INFO: Update rejected (update has been deferred).\n");
        break;

    case SysEvent_Status_Complete:
    default:
        Log_Debug("ERROR: Unexpected status %d.\n", status);
        terminationRequired = true;
        break;
    }

    Log_Debug("\n");

    UpdatePendingStatusLed();
}

/// <summary>
///     Convert the supplied system event status to a human-readable string.
/// </summary>
/// <param name="status">The status.</param>
/// <returns>String representation of the supplied status.</param>
static const char *EventStatusToString(SysEvent_Status status)
{
    switch (status) {
    case SysEvent_Status_Invalid:
        return "Invalid";
    case SysEvent_Status_Pending:
        return "Pending";
    case SysEvent_Status_Final:
        return "Final";
    case SysEvent_Status_Rejected:
        return "Rejected";
    case SysEvent_Status_Complete:
        return "Completed";
    default:
        return "Unknown";
    }
}

/// <summary>
///     Convert the supplied update type to a human-readable string.
/// </summary>
/// <param name="updateType">The update type.</param>
/// <returns>String representation of the supplied update type.</param>
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

/// <summary>
///     Register to be notified when application updates are available.
///     The EPOLL event handler will run when a system event occurs.
///     After calling this function, call <see cref="FreeSysEventHandler" /> to
///     free any resources.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int SetUpSysEventHandler(void)
{
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("ERROR: could not create event loop\n");
        return -1;
    }

    updateEventReg = SysEvent_RegisterForEventNotifications(eventLoop, SysEvent_Events_Update,
                                                            UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    // The entire event loop has a single readiness file descriptor.
    // When that descriptor is signalled, EventLoop_Run must be called
    // to handle the specific event.
    eventLoopFd = EventLoop_GetWaitDescriptor(eventLoop);
    if (eventLoopFd == -1) {
        Log_Debug("ERROR: Could not get event loop descriptor: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    static struct EventData sysEventHandler = {.eventHandler = SysEventHandler};

    if (RegisterEventHandlerToEpoll(epollFd, eventLoopFd, &sysEventHandler, EPOLLIN) == -1) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Free any resources which were successfully allocated with
///     <see cref="SetUpSysEventHandler" />.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void FreeSysEventHandler(void)
{
    if (updateEventReg != NULL) {
        SysEvent_UnregisterForEventNotifications(updateEventReg);
    }

    if (eventLoop != NULL) {
        EventLoop_Close(eventLoop);
    }

    if (eventLoopFd != -1) {
        UnregisterEventHandlerFromEpoll(epollFd, eventLoopFd);
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action = {.sa_handler = TerminationHandler};
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    // Open LEDs for accept mode status.
    acceptLedRedFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedRedFd < 0) {
        Log_Debug("ERROR: Could not open accept red LED: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    acceptLedGreenFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedGreenFd < 0) {
        Log_Debug("ERROR: Could not open accept green LED: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    acceptLedBlueFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedBlueFd < 0) {
        Log_Debug("ERROR: Could not open accept blue LED: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    UpdateAcceptModeLed();

    // Open application update pending LED.
    pendingUpdateLedFd =
        GPIO_OpenAsOutput(SAMPLE_PENDING_UPDATE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (pendingUpdateLedFd < 0) {
        Log_Debug("ERROR: Could not open update pending blue LED: %s (%d).\n", strerror(errno),
                  errno);
        return -1;
    }

    UpdatePendingStatusLed();

    // Open button and timer to check for button press.
    buttonFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonFd < 0) {
        Log_Debug("ERROR: Could not open sample button 1: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    static const struct timespec buttonCheckInterval = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};
    static struct EventData buttonEventData = {.eventHandler = ButtonTimerEventHandler};
    timerFd = CreateTimerFdAndAddToEpoll(epollFd, &buttonCheckInterval, &buttonEventData, EPOLLIN);

    if (SetUpSysEventHandler() == -1) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    SwitchOffAcceptModeLed();
    SwitchOffPendingStatusLed();

    FreeSysEventHandler();

    CloseFdAndPrintError(pendingUpdateLedFd, "pendingUpdateLedFd");

    CloseFdAndPrintError(buttonFd, "buttonFd");
    CloseFdAndPrintError(timerFd, "timerFd");

    CloseFdAndPrintError(acceptLedRedFd, "acceptLedRedFd");
    CloseFdAndPrintError(acceptLedGreenFd, "acceptLedGreenFd");
    CloseFdAndPrintError(acceptLedBlueFd, "acceptLedBlueFd");

    CloseFdAndPrintError(epollFd, "epollFd");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(void)
{
    Log_Debug("INFO: Application starting\n");
    Log_Debug("INFO: Press button to allow the deferral.\n");

    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("INFO: Application exiting\n");
}
