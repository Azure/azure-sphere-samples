/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use ADC (Analog to Digital
// Conversion).
// The sample opens an ADC controller which is connected to a potentiometer. Adjusting the
// potentiometer will change the displayed values.
//
// It uses the API for the following Azure Sphere application libraries:
// - ADC (Analog to Digital Conversion)
// - log (messages shown in Visual Studio's Device Output window during debugging)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/adc.h>
#include <applibs/log.h>

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed by using the project property "Target Hardware Definition Directory".
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

// File descriptors - initialized to invalid value
static int epollFd = -1;
static int adcControllerFd = -1;
static int pollTimerFd = -1;

// The size of a sample in bits
static int sampleBitCount = -1;

// The maximum voltage
static float sampleMaxVoltage = 2.5f;

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
///     Handle polling timer event: takes a single reading from ADC channelId,
///     every second, outputting the result.
/// </summary>
static void AdcPollingEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(pollTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    uint32_t value;
    int result = ADC_Poll(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL, &value);
    if (result < -1) {
        Log_Debug("ADC_Poll failed with error: %s (%d)\n", strerror(errno), errno);
        terminationRequired = true;
        return;
    }

    float voltage = ((float)value * sampleMaxVoltage) / (float)((1 << sampleBitCount) - 1);
    Log_Debug("The out sample value is %.3f V\n", voltage);
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData adcPollingEventData = {.eventHandler = &AdcPollingEventHandler};

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

    adcControllerFd = ADC_Open(SAMPLE_POTENTIOMETER_ADC_CONTROLLER);
    if (adcControllerFd < 0) {
        Log_Debug("ADC_Open failed with error: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    sampleBitCount = ADC_GetSampleBitCount(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL);
    if (sampleBitCount == -1) {
        Log_Debug("ADC_GetSampleBitCount failed with error : %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    if (sampleBitCount == 0) {
        Log_Debug("ADC_GetSampleBitCount returned sample size of 0 bits.\n");
        return -1;
    }

    int result = ADC_SetReferenceVoltage(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL,
                                         sampleMaxVoltage);
    if (result < 0) {
        Log_Debug("ADC_SetReferenceVoltage failed with error : %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    struct timespec adcCheckPeriod = {.tv_sec = 1, .tv_nsec = 0};
    pollTimerFd =
        CreateTimerFdAndAddToEpoll(epollFd, &adcCheckPeriod, &adcPollingEventData, EPOLLIN);
    if (pollTimerFd < 0) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(pollTimerFd, "Timer");
    CloseFdAndPrintError(adcControllerFd, "ADC");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("ADC application starting.\n");
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