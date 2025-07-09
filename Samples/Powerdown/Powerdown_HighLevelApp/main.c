/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates an application blinking a led,
// waiting for updates, and going into powerdown mode.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - powermanagement (enter powerdown mode, reboot the device)
// - sysevent (receive notification of, defer, and accept pending application update)
// - eventloop (system invokes handlers for timer events)
// - storage (to maintain data over device reboot)
// - networking (network ready)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/eventloop.h>
#include <applibs/sysevent.h>
#include <applibs/powermanagement.h>
#include <applibs/storage.h>
#include <applibs/networking.h>

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

    ExitCode_WriteProgramStateToMutableFile_OpenFile = 2,

    ExitCode_ReadProgramStateFromMutableFile_OpenFile = 3,

    ExitCode_ComputeTimeDifference_Fail = 4,

    ExitCode_WaitForUpdatesDownload_Consume = 5,

    ExitCode_WaitForUpdatesCheckTimer_Consume = 6,

    ExitCode_BusinessLogicTimer_Consume = 7,

    ExitCode_BlinkingTimer_Consume = 8,
    ExitCode_BlinkingTimer_SetValue = 9,

    ExitCode_UpdateCallback_SetBlinkPeriod = 10,
    ExitCode_UpdateCallback_SetValue = 11,
    ExitCode_UpdateCallback_Reboot = 12,
    ExitCode_UpdateCallback_GetUpdateEvent = 13,
    ExitCode_UpdateCallback_InvalidUpdateType = 14,
    ExitCode_UpdateCallback_UnexpectedEvent = 15,

    ExitCode_Powerdown_Fail = 16,

    ExitCode_Init_UpdateStartedTimer = 17,

    ExitCode_Init_RedLed = 18,
    ExitCode_Init_GreenLed = 19,
    ExitCode_Init_EventLoop = 20,
    ExitCode_Init_RegisterEvent = 21,
    ExitCode_Init_CreateBlinkingTimer = 22,
    ExitCode_Init_CreateBusinessLogicTimer = 23,
    ExitCode_Init_SetBusinessLogicTimer = 24,
    ExitCode_Init_CreateWaitForUpdatesCheckTimer = 25,
    ExitCode_Init_SetWaitForUpdatesCheckTimer = 26,
    ExitCode_Init_CreateWaitForUpdatesDownloadTimer = 27,
    ExitCode_Init_SetWaitForUpdatesDownloadTimer = 28,

    ExitCode_TriggerPowerdown_Success = 29,

    ExitCode_TriggerReboot_Success = 30,

    ExitCode_Main_EventLoopFail = 31,

    ExitCode_IsNetworkingReady_Failed = 32,

    ExitCode_BusinessLogicTimer_SetValue = 33
} ExitCode;

static volatile sig_atomic_t exitCode = ExitCode_Success;

// Application update events are received via an event loop
static EventLoop *eventLoop = NULL;
static EventRegistration *updateEventReg = NULL;

// The update state the system is in
SysEvent_Events currentUpdateState = SysEvent_Events_None;

static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context);

// Value read from MutableStorage
static time_t lastUpdateTimestamp;

// Current system time: the clock must be synchronized
static time_t currentTimestamp;

// The interval in seconds after which the device should wait for an update
static const double updateCheckIntervalInSeconds = 120.0;

static void UpdateTime(time_t *outputCurrentTime);
static void ReadProgramStateFromMutableFile(void);
static void WriteProgramStateToMutableFile(void);

// SAMPLE_RGBLED_RED will blink for 60 seconds and then the application will power down, unless it
// needs to wait for update-related processing.
static EventLoopTimer *businessLogicCompleteTimer = NULL;
static const struct timespec businessLogicCompleteTimerInterval = {.tv_sec = 60, .tv_nsec = 0};

static void BusinessLogicTimerEventHandler(EventLoopTimer *timer);

// Wait extra time to check for updates
static EventLoopTimer *waitForUpdatesCheckTimer = NULL;
static const struct timespec waitForUpdatesCheckTimerInterval = {.tv_sec = 120, .tv_nsec = 0};

static void WaitForUpdatesCheckTimerEventHandler(EventLoopTimer *timer);

// Wait extra time for the download to finish
static EventLoopTimer *waitForUpdatesToDownloadTimer = NULL;
static const struct timespec waitForUpdatesToDownloadTimerInterval = {.tv_sec = 300, .tv_nsec = 0};
static const struct timespec blinkIntervalWaitForUpdates = {.tv_sec = 0,
                                                            .tv_nsec = 500 * 1000 * 1000};

static void WaitForUpdatesDownloadTimerEventHandler(EventLoopTimer *timer);

// The status mode LED shows whether the application completes its business logic
// (SAMPLE_RGBLED_RED) or waits for updates (SAMPLE_RGBLED_GREEN).
static int blinkingLedRedFd = -1;
static int waitingUpdatesLedGreenFd = -1;

// By default, the system doesn't wait for updates
static bool isBusinessLogicComplete = false;
static EventLoopTimer *blinkTimer = NULL;
static const struct timespec blinkIntervalBusinessLogic = {.tv_sec = 0,
                                                           .tv_nsec = 125 * 1000 * 1000};
static GPIO_Value_Type ledState = GPIO_Value_High;

static void BlinkingLedTimerEventHandler(EventLoopTimer *timer);

// This constant defines the maximum time (in seconds) the device can be in powerdown mode. A value
// of less than 2 seconds will cause the device to resume from powerdown immediately, behaving like
// a reboot.
static const unsigned int powerdownResidencyTime = 10;

static void TriggerReboot(void);
static void TriggerPowerdown(void);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void SwitchOffLeds(void);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Updates the outputCurrentTime parameter with the current time.
/// </summary>
/// <param name="outputCurrentTime">Stores current time</param>
static void UpdateTime(time_t *outputCurrentTime)
{
    char timeBuf[64];
    time(outputCurrentTime);
    struct tm *tm = gmtime(outputCurrentTime);
    if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T", tm) != 0) {
        Log_Debug("INFO: Current time: %s\n", timeBuf);
    }
}

/// <summary>
///     Write lastUpdateTimestamp to the persistent data file.
/// </summary>
static void WriteProgramStateToMutableFile(void)
{
    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_WriteProgramStateToMutableFile_OpenFile;
        return;
    }

    ssize_t ret = write(fd, &lastUpdateTimestamp, sizeof(lastUpdateTimestamp));
    Log_Debug("INFO: Wrote lastUpdateTimestamp = %li\n", lastUpdateTimestamp);
    if (ret == -1) {
        // If the file has reached the maximum size specified in the application manifest,
        // then -1 will be returned with errno EDQUOT (122)
        Log_Debug("ERROR: An error occurred while writing to mutable file:  %s (%d).\n",
                  strerror(errno), errno);
    } else if (ret < sizeof(lastUpdateTimestamp)) {
        // For simplicity, this sample logs an error here. In the general case, this should be
        // handled by retrying the write with the remaining data until all the data has been
        // written.
        Log_Debug("ERROR: Only wrote %zd of %zu bytes requested\n", ret,
                  sizeof(lastUpdateTimestamp));
    }

    close(fd);
}

/// <summary>
///     Read lastUpdateTimestamp from the persistent data file. If the file doesn't
///     exist, set lastUpdateTimestamp to 0.
/// </summary>
static void ReadProgramStateFromMutableFile(void)
{
    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ReadProgramStateFromMutableFile_OpenFile;
        return;
    }

    lastUpdateTimestamp = 0;
    ssize_t ret = read(fd, &lastUpdateTimestamp, sizeof(lastUpdateTimestamp));

    Log_Debug("INFO: Read lastUpdateTimestamp = %li\n", lastUpdateTimestamp);
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while reading lastUpdateTimestamp:  %s (%d).\n",
                  strerror(errno), errno);
    }
    close(fd);

    if (ret < sizeof(lastUpdateTimestamp)) {
        lastUpdateTimestamp = 0;
    }

    char timeBuf[64];
    struct tm *tm = gmtime(&lastUpdateTimestamp);
    if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T", tm) != 0) {
        Log_Debug("INFO: Last update time: %s\n", timeBuf);
    }
}

/// <summary>
///     Waits for updates to download.
/// </summary>
static void WaitForUpdatesDownloadTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_WaitForUpdatesDownload_Consume;
        return;
    }

    Log_Debug("INFO: Wait for update download timed out. Powering down.\n");

    exitCode = ExitCode_TriggerPowerdown_Success;
}

/// <summary>
///     This handler runs when the period allowed for update checking has expired, and triggers
///     power down unless an update has begun.
/// </summary>
static void WaitForUpdatesCheckTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_WaitForUpdatesCheckTimer_Consume;
        return;
    }

    // An update download is in progress.
    if (currentUpdateState == SysEvent_Events_UpdateStarted) {
        return;
    }

    bool isNetworkReady;
    int result = Networking_IsNetworkingReady(&isNetworkReady);
    if (result == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_IsNetworkingReady_Failed;
        return;
    }

    if (isNetworkReady) {
        Log_Debug(
            "INFO: Wait for update check timed out, and there is no update download in progress. "
            "Powering down.\n");
    } else {
        Log_Debug(
            "WARNING: Wait for update check timed out, and there is no update download "
            "in progress. The device is not connected to any network. Powering down.\n");
    }

    // Set program state indicating that power down should occur.
    exitCode = ExitCode_TriggerPowerdown_Success;
}

/// <summary>
///     If the waiting time has expired and there are no updates downloading put the system in
///     powerdown mode. Otherwise wait for updates before going into powerdown mode.
/// </summary>
static void BusinessLogicTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_BusinessLogicTimer_Consume;
        return;
    }

    Log_Debug("INFO: Finished business logic.\n");
    isBusinessLogicComplete = true;

    // Switch off the Red Led
    int result = GPIO_SetValue(blinkingLedRedFd, GPIO_Value_High);
    if (result == -1) {
        Log_Debug("ERROR: GPIO_SetValue failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_BusinessLogicTimer_SetValue;
        return;
    }

    double checkUpdateIntervalSec = difftime(currentTimestamp, lastUpdateTimestamp);

    // the device should wait for an update if the `checkUpdateIntervalSec` interval
    // has passed or if there is an inconsistency between the currentTimestamp and
    // the lastUpdateTimestamp
    bool shouldWaitForUpdate = false;
    if (checkUpdateIntervalSec > updateCheckIntervalInSeconds || checkUpdateIntervalSec < 0) {
        shouldWaitForUpdate = true;
    }

    if (currentUpdateState == SysEvent_Events_UpdateStarted || shouldWaitForUpdate) {
        return;
    }

    exitCode = ExitCode_TriggerPowerdown_Success;
}

/// <summary>
///     Handle LED timer event: blink LED.
/// </summary>
static void BlinkingLedTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_BlinkingTimer_Consume;
        return;
    }

    // The blink interval has elapsed, so toggle the LED state
    // The LED is active-low so GPIO_Value_Low is on and GPIO_Value_High is off
    ledState = (ledState == GPIO_Value_Low ? GPIO_Value_High : GPIO_Value_Low);
    int ledFd = isBusinessLogicComplete ? waitingUpdatesLedGreenFd : blinkingLedRedFd;
    int result = GPIO_SetValue(ledFd, ledState);
    if (result != 0) {
        Log_Debug("ERROR: Could not set LED output value: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_BlinkingTimer_SetValue;
        return;
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
    // Update current state
    currentUpdateState = event;

    switch (event) {
    case SysEvent_Events_NoUpdateAvailable: {
        Log_Debug("INFO: Update check finished. No updates available\n");

        UpdateTime(&lastUpdateTimestamp);
        WriteProgramStateToMutableFile();

        if (isBusinessLogicComplete) {
            exitCode = ExitCode_TriggerPowerdown_Success;
        }
        break;
    }

    // Downloading updates has started. Change the blink interval to indicate this event has
    // occured, and keep waiting.
    case SysEvent_Events_UpdateStarted: {
        Log_Debug("INFO: Updates have started downloading\n");
        if (SetEventLoopTimerPeriod(blinkTimer, &blinkIntervalWaitForUpdates) != 0) {
            exitCode = ExitCode_UpdateCallback_SetBlinkPeriod;
        }
        break;
    }

    // Updates are ready for install
    case SysEvent_Events_UpdateReadyForInstall: {
        Log_Debug("INFO: Update download finished and is ready for install.\n");

        // Stop LED blinking, and switch on the green LED, to indicate this event has occured.
        DisarmEventLoopTimer(blinkTimer);
        SwitchOffLeds();

        int result = GPIO_SetValue(waitingUpdatesLedGreenFd, GPIO_Value_Low);
        if (result == -1) {
            Log_Debug("ERROR: GPIO_SetValue failed: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_UpdateCallback_SetValue;
            break;
        }

        SysEvent_Info_UpdateData data;
        result = SysEvent_Info_GetUpdateData(info, &data);
        if (result == -1) {
            Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_UpdateCallback_GetUpdateEvent;
            break;
        }

        if (data.update_type == SysEvent_UpdateType_App) {
            Log_Debug("INFO: Application update. The device will powerdown.\n");

            UpdateTime(&lastUpdateTimestamp);
            WriteProgramStateToMutableFile();

            exitCode = ExitCode_TriggerPowerdown_Success;
        } else if (data.update_type == SysEvent_UpdateType_System) {
            Log_Debug("INFO: System update. The device will reboot.\n");
            exitCode = ExitCode_TriggerReboot_Success;
        } else {
            exitCode = ExitCode_UpdateCallback_InvalidUpdateType;
            Log_Debug("ERROR: ExitCode_UpdateCallback_InvalidUpdateType.\n");
        }

        break;
    }

    default:
        Log_Debug("ERROR: Unexpected event\n");
        exitCode = ExitCode_UpdateCallback_UnexpectedEvent;
        break;
    }

    Log_Debug("\n");
}

/// <summary>
///     Reboot the device.
/// </summary>
static void TriggerReboot(void)
{
    // Reboot the system
    int result = PowerManagement_ForceSystemReboot();
    if (result != 0) {
        Log_Debug("Error PowerManagement_ForceSystemReboot: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_UpdateCallback_Reboot;
    }
}

/// <summary>
///     Power down the device.
/// </summary>
static void TriggerPowerdown(void)
{
    // Put the device in the powerdown mode
    int result = PowerManagement_ForceSystemPowerDown(powerdownResidencyTime);
    if (result != 0) {
        Log_Debug("Error PowerManagement_ForceSystemPowerDown: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_Powerdown_Fail;
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
    struct sigaction action = {.sa_handler = TerminationHandler};
    sigaction(SIGTERM, &action, NULL);

    // Read the current time
    UpdateTime(&currentTimestamp);

    // Read the last update time
    ReadProgramStateFromMutableFile();

    // Open LEDs for accept mode status.
    blinkingLedRedFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blinkingLedRedFd < 0) {
        Log_Debug("ERROR: Could not open start red LED: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_RedLed;
    }

    waitingUpdatesLedGreenFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (waitingUpdatesLedGreenFd < 0) {
        Log_Debug("ERROR: Could not open check for updates green LED: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_Init_GreenLed;
    }

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    updateEventReg = SysEvent_RegisterForEventNotifications(eventLoop, SysEvent_Events_Mask,
                                                            UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_RegisterEvent;
    }

    blinkTimer = CreateEventLoopPeriodicTimer(eventLoop, &BlinkingLedTimerEventHandler,
                                              &blinkIntervalBusinessLogic);
    if (blinkTimer == NULL) {
        return ExitCode_Init_CreateBlinkingTimer;
    }

    businessLogicCompleteTimer =
        CreateEventLoopDisarmedTimer(eventLoop, &BusinessLogicTimerEventHandler);
    if (businessLogicCompleteTimer == NULL) {
        return ExitCode_Init_CreateBusinessLogicTimer;
    }
    int result =
        SetEventLoopTimerOneShot(businessLogicCompleteTimer, &businessLogicCompleteTimerInterval);
    if (result != 0) {
        return ExitCode_Init_SetBusinessLogicTimer;
    }

    waitForUpdatesCheckTimer =
        CreateEventLoopDisarmedTimer(eventLoop, &WaitForUpdatesCheckTimerEventHandler);
    if (waitForUpdatesCheckTimer == NULL) {
        return ExitCode_Init_CreateWaitForUpdatesCheckTimer;
    }
    result = SetEventLoopTimerOneShot(waitForUpdatesCheckTimer, &waitForUpdatesCheckTimerInterval);
    if (result != 0) {
        exitCode = ExitCode_Init_SetWaitForUpdatesCheckTimer;
    }

    waitForUpdatesToDownloadTimer =
        CreateEventLoopDisarmedTimer(eventLoop, &WaitForUpdatesDownloadTimerEventHandler);
    if (waitForUpdatesToDownloadTimer == NULL) {
        return ExitCode_Init_CreateWaitForUpdatesDownloadTimer;
    }
    result = SetEventLoopTimerOneShot(waitForUpdatesToDownloadTimer,
                                      &waitForUpdatesToDownloadTimerInterval);
    if (result != 0) {
        exitCode = ExitCode_Init_SetWaitForUpdatesDownloadTimer;
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
///     Switch off the Leds.
/// </summary>
static void SwitchOffLeds(void)
{
    if (blinkingLedRedFd != -1) {
        GPIO_SetValue(blinkingLedRedFd, GPIO_Value_High);
    }

    if (waitingUpdatesLedGreenFd != -1) {
        GPIO_SetValue(waitingUpdatesLedGreenFd, GPIO_Value_High);
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("INFO: ClosePeripheralsAndHandlers\n");

    SwitchOffLeds();

    DisposeEventLoopTimer(blinkTimer);
    DisposeEventLoopTimer(businessLogicCompleteTimer);
    DisposeEventLoopTimer(waitForUpdatesCheckTimer);
    DisposeEventLoopTimer(waitForUpdatesToDownloadTimer);
    SysEvent_UnregisterForEventNotifications(updateEventReg);
    EventLoop_Close(eventLoop);

    CloseFdAndPrintError(blinkingLedRedFd, "SAMPLE_RGBLED_RED");
    CloseFdAndPrintError(waitingUpdatesLedGreenFd, "SAMPLE_RGBLED_GREEN");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(void)
{
    Log_Debug("INFO: Powerdown application starting...\n");

    exitCode = InitPeripheralsAndHandlers();

    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
            Log_Debug("Error: eventloop failed with error code: %d %d %s\n", result, errno,
                      strerror(errno));
        }
    }

    // Close peripherals and turn the LEDs off
    ClosePeripheralsAndHandlers();

    if (exitCode == ExitCode_TriggerPowerdown_Success) {
        exitCode = ExitCode_Success;
        TriggerPowerdown();
    } else if (exitCode == ExitCode_TriggerReboot_Success) {
        exitCode = ExitCode_Success;
        TriggerReboot();
    }

    return exitCode;
}