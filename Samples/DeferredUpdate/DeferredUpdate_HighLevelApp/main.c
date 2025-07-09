/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates an application receiving
// notifications for a pending application update, and then deferring that update.
// On the MT3620 RDB,
// LED 2 is green when the update should be deferred, and yellow when it should be applied.
// Press SAMPLE_BUTTON_1 to toggle between these modes.
// LED 3 is lit up blue when an OTA update is available.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - sysevent (receive notification of, defer, and accept pending
// application update)

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

// The following #include imports a "sample appliance" hardware definition. This provides a set of
// named constants such as SAMPLE_BUTTON_1 which are used when opening the peripherals, rather
// that using the underlying pin names. This enables the same code to target different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio. To target different hardware, you'll
// need to update the TARGET_HARDWARE variable in CMakeLists.txt - see instructions in that file.
//
// You can also use hardware definitions related to all other peripherals on your dev board because
// the sample_appliance header file recursively includes underlying hardware definition headers.
// See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
#include <hw/sample_appliance.h>

#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ButtonTimer_Consume = 2,
    ExitCode_ButtonTimer_GetValue = 3,

    ExitCode_UpdateCallback_UnexpectedEvent = 4,
    ExitCode_UpdateCallback_GetUpdateEvent = 5,
    ExitCode_UpdateCallback_DeferEvent = 6,
    ExitCode_UpdateCallback_FinalUpdate = 7,
    ExitCode_UpdateCallback_UnexpectedStatus = 8,
    ExitCode_SetUpSysEvent_EventLoop = 9,
    ExitCode_SetUpSysEvent_RegisterEvent = 10,
    ExitCode_SetUpSysEvent_ButtonTimer = 11,

    ExitCode_Init_OpenRedLed = 12,
    ExitCode_Init_OpenGreenLed = 13,
    ExitCode_Init_OpenBlueLed = 14,
    ExitCode_Init_OpenPendingLed = 15,
    ExitCode_Init_OpenButton = 16,

    ExitCode_Main_EventLoopFail = 17,

    ExitCode_SigTerm_SetSigMaskFailure = 18,
    ExitCode_SigTerm_Timeout = 19,
    ExitCode_SigTerm_OtherFailure = 20,
    ExitCode_SigTerm_UnexpectedSignal = 21
} ExitCode;

static volatile sig_atomic_t exitCode = ExitCode_Success;

static EventLoop *eventLoop = NULL;

// The accept mode LED triplet shows whether updates are allowed SAMPLE_RGBLED (yellow)
// will be on, or deferred (green) will be on.
static int acceptLedRedFd = -1;
static int acceptLedGreenFd = -1;
static int acceptLedBlueFd = -1;

// Press the button to toggle between accept or defer updates.
static EventLoopTimer *buttonPollTimer = NULL;
static int buttonFd = -1;
static bool acceptUpdate = false;

static void UpdateAcceptModeLed(void);
static void SwitchOffAcceptModeLed(void);
static void ButtonTimerEventHandler(EventLoopTimer *timer);

// The pending update LED lights up the application is notified of a pending update.
static int pendingUpdateLedFd = -1;

static void SwitchOffPendingStatusLed(void);
static void UpdatePendingStatusLed(void);

// Application update events are received via an event loop.
static EventRegistration *updateEventReg = NULL;
static bool pendingUpdate = false;

static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context);
static const char *EventStatusToString(SysEvent_Status status);
static const char *UpdateTypeToString(SysEvent_UpdateType updateType);

static void TerminationHandler(int signalNumber);

static ExitCode SetUpSysEventHandler(void);
static void FreeSysEventHandler(void);

static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);
static ExitCode WaitForSigTerm(time_t);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Set the accept mode SAMPLE_RGBLED to yellow (green and red will be on) if
///     updates will be accepted, and to green if they should be deferred.
/// </summary>
static void UpdateAcceptModeLed(void)
{
    bool red, green, blue;

    // If updates are allowed, SAMPLE_RGBLED will be yellow (green + red)...
    if (acceptUpdate) {
        red = true;
        green = true;
        blue = false;
    }
    // ...else if updates should be deferred, SAMPLE_RGBLED will be green.
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
/// <param name="timer">Event loop timer.</param>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    static GPIO_Value_Type buttonState = GPIO_Value_High;

    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(buttonFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetValue;
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
        SysEvent_ResumeEvent(SysEvent_Events_UpdateReadyForInstall);
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
///     This function matches the SysEvent_EventsCallback signature, and is invoked
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context)
{
    if (event != SysEvent_Events_UpdateReadyForInstall) {
        Log_Debug("ERROR: unexpected event: 0x%x\n", event);
        exitCode = ExitCode_UpdateCallback_UnexpectedEvent;
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
        exitCode = ExitCode_UpdateCallback_GetUpdateEvent;
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
            result = SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, 1);
        }

        if (result == -1) {
            Log_Debug("ERROR: SysEvent_DeferEvent: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_UpdateCallback_DeferEvent;
        }
        break;

    case SysEvent_Status_Final:
        Log_Debug("INFO: Final update. App will update in 10 seconds.\n");
        // Terminate app before it is forcibly shut down and replaced.
        // The application may be restarted before the update is applied.
        exitCode = ExitCode_UpdateCallback_FinalUpdate;
        break;

    case SysEvent_Status_Deferred:
        Log_Debug("INFO: Update deferred.\n");
        break;

    case SysEvent_Status_Complete:
    default:
        Log_Debug("ERROR: Unexpected status %d.\n", status);
        exitCode = ExitCode_UpdateCallback_UnexpectedStatus;
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
    case SysEvent_Status_Deferred:
        return "Deferred";
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
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode SetUpSysEventHandler(void)
{
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("ERROR: could not create event loop\n");
        return ExitCode_SetUpSysEvent_EventLoop;
    }

    updateEventReg = SysEvent_RegisterForEventNotifications(
        eventLoop, SysEvent_Events_UpdateReadyForInstall, UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        return ExitCode_SetUpSysEvent_RegisterEvent;
    }

    static const struct timespec buttonPollInterval100Ms = {.tv_sec = 0,
                                                            .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler, &buttonPollInterval100Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_SetUpSysEvent_ButtonTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Free any resources which were successfully allocated with
///     <see cref="SetUpSysEventHandler" />.
/// </summary>
static void FreeSysEventHandler(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    SysEvent_UnregisterForEventNotifications(updateEventReg);
    EventLoop_Close(eventLoop);
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
    struct sigaction action = {.sa_handler = TerminationHandler};
    sigaction(SIGTERM, &action, NULL);

    // Open LEDs for accept mode status.
    acceptLedRedFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedRedFd == -1) {
        Log_Debug("ERROR: Could not open accept SAMPLE_RGBLED_RED: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_OpenRedLed;
    }

    acceptLedGreenFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedGreenFd == -1) {
        Log_Debug("ERROR: Could not open accept SAMPLE_RGBLED_GREEN: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_OpenGreenLed;
    }

    acceptLedBlueFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (acceptLedBlueFd == -1) {
        Log_Debug("ERROR: Could not open accept SAMPLE_RGBLED_BLUE: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_OpenBlueLed;
    }

    UpdateAcceptModeLed();

    // Open application update pending LED.
    pendingUpdateLedFd =
        GPIO_OpenAsOutput(SAMPLE_PENDING_UPDATE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (pendingUpdateLedFd == -1) {
        Log_Debug("ERROR: Could not open update SAMPLE_PENDING_UPDATE_LED: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_Init_OpenPendingLed;
    }

    UpdatePendingStatusLed();

    // Open SAMPLE_BUTTON_1 GPIO and timer to check for button press.
    buttonFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OpenButton;
    }

    ExitCode localExitCode = SetUpSysEventHandler();

    return localExitCode;
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
    SwitchOffAcceptModeLed();
    SwitchOffPendingStatusLed();

    FreeSysEventHandler();

    CloseFdAndPrintError(pendingUpdateLedFd, "pendingUpdateLedFd");

    CloseFdAndPrintError(buttonFd, "buttonFd");

    CloseFdAndPrintError(acceptLedRedFd, "acceptLedRedFd");
    CloseFdAndPrintError(acceptLedGreenFd, "acceptLedGreenFd");
    CloseFdAndPrintError(acceptLedBlueFd, "acceptLedBlueFd");
}

/// <summary>
///     Wait for SIGTERM (or timeout)
/// </summary>
/// <param name="timeoutSecs">Timeout period in seconds</param>
static ExitCode WaitForSigTerm(time_t timeoutSecs)
{
    sigset_t sigtermSet, oldMask;

    sigemptyset(&sigtermSet);
    sigaddset(&sigtermSet, SIGTERM);

    // Block SIGTERM - disables the existing SIGTERM handler
    if (sigprocmask(SIG_BLOCK, &sigtermSet, &oldMask) == -1) {
        Log_Debug("ERROR: Could not set process signal mask: %d (%s)", errno, strerror(errno));
        return ExitCode_SigTerm_SetSigMaskFailure;
    }

    struct timespec timeout = {.tv_sec = timeoutSecs, .tv_nsec = 0};

    int result = sigtimedwait(&sigtermSet, NULL, &timeout);

    switch (result) {
    case SIGTERM:
        Log_Debug("INFO: SIGTERM received; exiting.\n");
        return ExitCode_Success;
    case -1:
        if (errno == EAGAIN) {
            Log_Debug("ERROR: Timed out waiting for SIGTERM\n");
            return ExitCode_SigTerm_Timeout;
        } else {
            Log_Debug("ERROR: Waiting for SIGTERM: %d (%s)\n", errno, strerror(errno));
            return ExitCode_SigTerm_OtherFailure;
        }
    default:
        Log_Debug("WARNING: Unexpected signal received when waiting for SIGTERM: %d\n", result);
        return ExitCode_SigTerm_UnexpectedSignal;
    }
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(void)
{
    Log_Debug("INFO: Application starting\n");
    Log_Debug("INFO: Press SAMPLE_BUTTON_1 to allow the deferral.\n");

    exitCode = InitPeripheralsAndHandlers();

    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();

    if (exitCode == ExitCode_UpdateCallback_FinalUpdate) {
        Log_Debug("INFO: Waiting for SIGTERM\n");

        // SIGTERM should arrive in 10 seconds; allow 20 to be sure.
        exitCode = WaitForSigTerm(20);
    }

    Log_Debug("INFO: Application exiting\n");

    return exitCode;
}
