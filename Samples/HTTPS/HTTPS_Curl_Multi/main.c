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

#include "epoll_timerfd_utilities.h"
#include "web_client.h"
#include "exitcode_curlmulti.h"

// By default, this sample targets hardware that follows the MT3620 Reference
// Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. See
// https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include "ui.h"

// File descriptors - initialized to invalid value
static int epollFd = -1;

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

    epollFd = CreateEpollFd();
    if (epollFd == -1) {
        return ExitCode_Init_Epoll;
    }

    ExitCode localExitCode = Ui_Init(epollFd);
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    localExitCode = WebClient_Init(epollFd);
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
    CloseFdAndPrintError(epollFd, "Epoll");

    // Release resources.
    WebClient_Fini();
    Ui_Fini();
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

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");
    return exitCode;
}
