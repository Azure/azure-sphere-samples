/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use Pulse Width
// Modulation (PWM).
// The sample opens a PWM controller. Adjusting the duty cycle will change the
// brightness of an LED.
//
// It uses the API for the following Azure Sphere application libraries:
// - pwm (Pulse Width Modulation)
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for timer events)

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <applibs/log.h>
#include <applibs/pwm.h>
#include <applibs/eventloop.h>

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

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_TurnOffChannel_Apply = 2,
    ExitCode_StepTimerHandler_Consume = 3,
    ExitCode_StepTimerHandler_Apply = 4,
    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_StepTimer = 6,
    ExitCode_Init_PwmOpen = 7,
    ExitCode_Main_EventLoopFail = 8
} ExitCode;

// File descriptors - initialized to invalid value
static int pwmFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *stepTimer = NULL;

// Each time the step timer fires (every stepInterval100Ms), we increase the current duty cycle
// (dutyCycleNs) by the step increment (stepIncrement), until the full duty cycle
// (fullCycleNs) is reached, at which point the current duty cycle is reset to 0.
// Supported PWM periods and duty cycles will vary depending on the hardware used;
// consult your specific device’s datasheet for details.
static const unsigned int fullCycleNs = 20 * 1000;
static const unsigned int stepIncrementNs = 1000;
static unsigned int dutyCycleNs = 0;

// The polarity is inverted because LEDs are driven low
static PwmState ledPwmState = {.period_nsec = fullCycleNs,
                               .polarity = PWM_Polarity_Inversed,
                               .dutyCycle_nsec = 0,
                               .enabled = true};

// Timer state variables
static const struct timespec stepInterval100Ms = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static ExitCode TurnAllChannelsOff(void);
static void StepTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
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
///     Turns all channels off for the opened controller.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode TurnAllChannelsOff(void)
{

    for (unsigned int i = MT3620_PWM_CHANNEL0; i <= MT3620_PWM_CHANNEL3; ++i) {
        int result = PWM_Apply(pwmFd, i, &ledPwmState);
        if (result != 0) {
            Log_Debug("PWM_Apply failed: result = %d, errno value: %s (%d)\n", result,
                      strerror(errno), errno);
            return ExitCode_TurnOffChannel_Apply;
        }
    }

    return ExitCode_Success;
}

/// <summary>
///     Handle LED timer event: change LED brightness.
/// </summary>
static void StepTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_StepTimerHandler_Consume;
        return;
    }

    // The step interval has elapsed, so increment the duty cycle.
    if (dutyCycleNs < fullCycleNs) {
        dutyCycleNs += stepIncrementNs;
    } else {
        dutyCycleNs = 0;
    }

    ledPwmState.dutyCycle_nsec = dutyCycleNs;

    int result = PWM_Apply(pwmFd, SAMPLE_LED_PWM_CHANNEL, &ledPwmState);
    if (result != 0) {
        Log_Debug("PWM_Apply failed: result = %d, errno: %s (%d)\n", result, strerror(errno),
                  errno);
        exitCode = ExitCode_StepTimerHandler_Apply;
        return;
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

    stepTimer = CreateEventLoopPeriodicTimer(eventLoop, &StepTimerEventHandler, &stepInterval100Ms);
    if (stepTimer == NULL) {
        return ExitCode_Init_StepTimer;
    }

    pwmFd = PWM_Open(SAMPLE_LED_PWM_CONTROLLER);
    if (pwmFd == -1) {
        Log_Debug(
            "Error opening SAMPLE_LED_PWM_CONTROLLER: %s (%d). Check that app_manifest.json "
            "includes the PWM used.\n",
            strerror(errno), errno);
        return ExitCode_Init_PwmOpen;
    }

    ExitCode localExitCode = TurnAllChannelsOff();
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
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
    DisposeEventLoopTimer(stepTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    if (pwmFd != -1) {
        // Leave the LED off
        TurnAllChannelsOff();
        CloseFdAndPrintError(pwmFd, "PwmFd");
    }
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Starting PWM Sample\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
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