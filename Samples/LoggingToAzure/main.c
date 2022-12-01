/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application demonstrates how to use Azure Sphere devices with Azure IoT
// services, using the Azure IoT C SDK.
//
// It implements a simulated thermometer device, with the following features:
// - Telemetry upload (simulated temperature, device moved events) using Azure IoT Hub events.
// - Reporting device state (serial number) using device twin/read-only properties.
// - Mutable device state (telemetry upload enabled) using device twin/writeable properties.
// - Alert messages invoked from Azure using device methods.
//
// It can be configured using the top-level CMakeLists.txt to connect either directly to an
// Azure IoT Hub, to an Azure IoT Edge device, or to use the Azure Device Provisioning service to
// connect to either an Azure IoT Hub, or an Azure IoT Central application. All connection types
// make use of the device certificate issued by the Azure Sphere security service to authenticate,
// and supply an Azure IoT PnP model ID on connection.
//
// It uses the following Azure Sphere libraries:
// - eventloop (system invokes handlers for timer events)
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - networking (network interface connection status)
//
// You will need to provide information in the application manifest to use this application. Please
// see README.md and the other linked documentation for full details.

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/applications.h>
#include <applibs/storage.h>
#include <applibs/log.h>
#include "eventloop_timer_utilities.h"
#include "exitcodes.h"
#include "options.h"
#include "connection.h"
#include "azure_iot.h"
#include "log_azure.h"
#include "utils.h"

#define APP_VERSION "v0.0.7"
#define MAX_IFACE_STRING_LENGTH 100
#define MAX_BOOT_STRING_ALLOC 30

static volatile sig_atomic_t exitCode = ExitCode_Success;

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// Interface callbacks
static void ExitCodeCallbackHandler(ExitCode ec);

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *telemetryTimer = NULL;

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
    Log_Debug("IoT Hub Debug Application starting.\n");

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    exitCode = Options_ParseArgs(argc, argv);

    if (exitCode != ExitCode_Success) {
        return exitCode;
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

    int fd = Storage_OpenMutableFile();

    if (fd) {
        char app_exit_msg[] = "App exited: ";
        char exit_code[12] = {0};

        Async_Safe_Itoa(exitCode, exit_code, 12);

        write(fd, app_exit_msg, sizeof(app_exit_msg));
        write(fd, exit_code, sizeof(exit_code));
    }

    close(fd);

    return exitCode;
}

static void ExitCodeCallbackHandler(ExitCode ec)
{
    exitCode = ec;
}

static void LogDebugCallbackHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }

    // It is important first boot diagnostic information is sent successfully.
    // Ensure we are connected to Azure before sending this information.
    if (AzureIoT_IsConnected()) {
        static bool loggedOsInformation = false;

        // log diagnostic information on first boot.
        if (!loggedOsInformation) {
            // report operating system version
            Applications_OsVersion osVersion;
            int ret = Applications_GetOsVersion(&osVersion);

            time_t now;
            time(&now);

            // report local time in UTC format.
            char datetime[DATETIME_BUFFER_SIZE] = {0};
            DateTime_UTC(datetime, DATETIME_BUFFER_SIZE, now);

            // build a string that reports the status of all
            // operable network interfaces,
            char ifaceString[MAX_IFACE_STRING_LENGTH] = {0};
            NetIfaces_ToString(ifaceString, MAX_IFACE_STRING_LENGTH, true, true);

            // also report any data stored in mutable storage
            // from the last run.
            int fd = Storage_OpenMutableFile();
            char *lastBootString = NULL;

            if (fd) {
                lastBootString = (char *)malloc(MAX_BOOT_STRING_ALLOC + 1);
                memset(lastBootString, 0, MAX_BOOT_STRING_ALLOC + 1);
                read(fd, lastBootString, MAX_BOOT_STRING_ALLOC);
            }

            // Log_Azure automatically initialises on first log.
            AzureIoT_Result logResult = Log_Azure(
                "OS Version: %s\nApplication version: %s\nNetwork interface status:\n\t%sLocal "
                "time: %s\nCrash info: %s",
                (ret == 0) ? osVersion.version : "Unknown", APP_VERSION, ifaceString, datetime,
                (lastBootString && *lastBootString != 0) ? lastBootString : "None");

            // ensure the diagnostic information was logged correctly.
            loggedOsInformation = logResult == AzureIoT_Result_OK;

            if (fd) {
                if (lastBootString)
                    free(lastBootString);

                close(fd);

                // clear crash data for next time on log success.
                if (loggedOsInformation)
                    Storage_DeleteMutableFile();
            }
        } else {
            size_t bytesToAlloc = (size_t)(rand() % 10) * 100;
            void *dummyAlloc = malloc(bytesToAlloc);
            size_t appMemoryUsage = Applications_GetTotalMemoryUsageInKB();

            // Log_Azure automatically initialises on first log.
            Log_Azure("Memory used %d Kb", appMemoryUsage);
            free(dummyAlloc);
        }
    } else {
        Log_Debug("Unable to connect to Azure\n");
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

    struct timespec debugPeriod = {.tv_sec = 10, .tv_nsec = 0};
    telemetryTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &LogDebugCallbackHandler, &debugPeriod);
    if (telemetryTimer == NULL) {
        return ExitCode_Init_TelemetryTimer;
    }

    void *connectionContext = Options_GetConnectionContext();

    // Log_Azure is portable and compatible with all AzureIoT samples.
    // a basic AzureIoTInitialize is used here to improve the clarity of the sample.

    // Log_Azure overrides the cloudtoDeviceCallbackFunction on first log. This is so
    // Azure logging can be enabled/disabled remotely.
    // If this behaviour is not desired, then manually call Log_Azure_Init(false)
    // before the first Log_Azure call occurs.
    AzureIoT_Callbacks callbacks = {.connectionStatusCallbackFunction = NULL,
                                    .deviceTwinReceivedCallbackFunction = NULL,
                                    .deviceTwinReportStateAckCallbackTypeFunction = NULL,
                                    .sendTelemetryCallbackFunction = NULL,
                                    .deviceMethodCallbackFunction = NULL,
                                    .cloudToDeviceCallbackFunction = NULL};

    return AzureIoT_Initialize(eventLoop, ExitCodeCallbackHandler, NULL, connectionContext,
                               callbacks);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(telemetryTimer);
    AzureIoT_Cleanup();
    Connection_Cleanup();
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");
}
