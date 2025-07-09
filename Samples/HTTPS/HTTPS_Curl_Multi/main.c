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
// - curl (URL transfer library)
// - gpio (digital input for button)
// - log (displays messages in the Device Output window during debugging)
// - networking (network ready)
// - storage (device storage interaction)

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>

#include "eventloop_timer_utilities.h"
#include "web_client.h"
#include "curlmulti.h"

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
#include "ui.h"

static void ParseCommandLineArguments(int argc, char *argv[]);

static EventLoop *eventLoop = NULL;

// By default, do not bypass proxy.
static bool bypassProxy = false;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

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

    localExitCode = WebClient_Init(eventLoop, bypassProxy);
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
///     Parse the command-line arguments given in the application manifest.
/// </summary>
static void ParseCommandLineArguments(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "BypassProxy", .has_arg = no_argument, .flag = NULL, .val = 'b'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "b", cmdLineOptions, NULL)) != -1) {
        switch (option) {
        case 'b':
            Log_Debug("Bypass Proxy\n");
            bypassProxy = true;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char **argv)
{
    Log_Debug("cURL multi interface based application starting.\n");
    Log_Debug(
        "Press SAMPLE_BUTTON_1 to initialize a set of parallel, asynchronous web transfers.\n");

    ParseCommandLineArguments(argc, argv);

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
