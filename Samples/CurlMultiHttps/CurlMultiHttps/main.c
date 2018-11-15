/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

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
#include "ui.h"

// This sample C application for Azure Sphere starts multiple concurrent web pages download
// using the cURL 'multi' interface. The response content is output as soon as it arrives.
// Pressing the button A initiates the web transfers. The communication happens over HTTP or
// HTTPS, as long as the certificate provided could validate the server identity.
// At the same time, LED1 blinks at a constant rate, demonstrating that the cURL 'multi' 
// interface is non-blocking.
//
// It uses the following Azure Sphere libraries:
// - gpio (digital input for button);
// - log (messages shown in Visual Studio's Device Output window during debugging);
// - storage (device storage interaction);
// - curl (URL transfer library).

// File descriptors - initialized to invalid value
static int epollFd = -1;

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
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    if ((Ui_Init(epollFd)) != 0) {
        return -1;
    }
    if ((WebClient_Init(epollFd)) != 0) {
        return -1;
    }
    return 0;
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
    Log_Debug("Press button A to initialize a set of parallel, asynchronous web transfers.\n");

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
