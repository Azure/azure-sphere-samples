/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere starts multiple concurrent web pages download
// using the cURL 'multi' interface. The response content is output as soon as it arrives.
// Pressing the SAMPLE_BUTTON_1 initiates the web transfers. The communication happens over
// HTTP or HTTPS, as long as the certificate provided could validate the server identity.
// At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi'
// interface is non-blocking.
//
// It uses the following Azure Sphere libraries:
// - gpio (digital input for button);
// - log (messages shown in Visual Studio's Device Output window during debugging);
// - storage (device storage interaction);
// - curl (URL transfer library).

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/networking.h>
#include <applibs/eventloop.h>

#include "eventloop_timer_utilities.h"
#include "web_client.h"
#include "curlmulti.h"

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_DIRECTORY argument in the call to
// azsphere_target_hardware_definition to "HardwareDefinitions/avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
#include "ui.h"

static EventLoop *eventLoop = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

// Network interface to use.
const char networkInterface[] = "wlan0";

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
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

    ExitCode localExitCode = Ui_Init(eventLoop);
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    localExitCode = WebClient_Init(eventLoop);
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
void ClosePeripheralsAndHandlers(void)
{
    // Release resources.
    WebClient_Fini();
    Ui_Fini();

    EventLoop_Close(eventLoop);
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char **argv)
{
    Log_Debug("cURL multi interface based application starting.\n");
    Log_Debug(
        "Press SAMPLE_BUTTON_1 to initialize a set of parallel, asynchronous web transfers.\n");

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
