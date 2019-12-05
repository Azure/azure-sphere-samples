/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use Pulse Width
// Modulation (PWM).
// The sample opens a PWM controller. Adjusting the duty cycle will change the
// brightness of an LED.
//
// It uses the API for the following Azure Sphere application libraries:
// - pwm (Pulse Width Modulation)
// - log (messages shown in Visual Studio's Device Output window during debugging)

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <applibs/log.h>
#include <applibs/pwm.h>

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

// This sample uses a single-thread event loop pattern, based on epoll and timerfd
#include "epoll_timerfd_utilities.h"

// File descriptors - initialized to invalid value
static int pwmFd = -1;
static int stepTimerFd = -1;
static int epollFd = -1;

// Each time the step timer fires (every stepIntervalNs), we increase the current duty cycle
// (dutyCycleNs) by the step increment (stepIncrementNs), until the full duty cycle
// (fullCycleNs) is reached, at which point the current duty cycle is reset to 0.
// Supported PWM periods and duty cycles will vary depending on the hardware used;
// consult your specific device’s datasheet for details.
static const unsigned int fullCycleNs = 20000;
static const unsigned int stepIncrementNs = 1000;
static unsigned int dutyCycleNs = 0;

// The polarity is inverted because LEDs are driven low
static PwmState ledPwmState = {.period_nsec = fullCycleNs,
                               .polarity = PWM_Polarity_Inversed,
                               .dutyCycle_nsec = 0,
                               .enabled = true};

// Timer state variables
static const struct timespec stepIntervalNs = {.tv_sec = 0, .tv_nsec = 100000000};

// Termination state
static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Turns all channels off for the opened controller.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int TurnAllChannelsOff(void)
{

    for (unsigned int i = MT3620_PWM_CHANNEL0; i <= MT3620_PWM_CHANNEL3; ++i) {
        int result = PWM_Apply(pwmFd, i, &ledPwmState);
        if (result != 0) {
            Log_Debug("PWM_Apply failed: result = %d, errno value: %s (%d)\n", result,
                      strerror(errno), errno);
            terminationRequired = true;
            return result;
        }
    }

    return 0;
}

/// <summary>
///     Handle LED timer event: change LED brightness.
/// </summary>
static void StepTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(stepTimerFd) != 0) {
        terminationRequired = true;
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
        terminationRequired = true;
    }
}

// Event handler data structures. Only the event handler field needs to be populated.
static EventData stepTimerEventData = {.eventHandler = &StepTimerEventHandler};

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    stepTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &stepIntervalNs, &stepTimerEventData, EPOLLIN);
    if (stepTimerFd < 0) {
        return -1;
    }

    pwmFd = PWM_Open(SAMPLE_LED_PWM_CONTROLLER);
    if (pwmFd == -1) {
        Log_Debug(
            "Error opening SAMPLE_LED_PWM_CONTROLLER: %s (%d). Check that app_manifest.json "
            "includes the PWM used.\n",
            strerror(errno), errno);
        return -1;
    }

    if (TurnAllChannelsOff()) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    // Leave the LED off
    TurnAllChannelsOff();

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(pwmFd, "PwmFd");
    CloseFdAndPrintError(stepTimerFd, "stepTimerFd");
    CloseFdAndPrintError(epollFd, "epollFd");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Starting PWM Sample\n");
    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");

    return 0;
}