/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/eventloop.h>

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
#include <hw/soda_machine.h>

#include "business_logic.h"
#include "cloud.h"
#include "debug_uart.h"
#include "eventloop_timer_utilities.h"
#include "exitcode.h"
#include "message_protocol.h"
#include "power.h"
#include "mcu_messaging.h"
#include "uart_transport.h"
#include "update.h"

static EventLoop *eventLoop = NULL;
static const char *scopeId = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

static void ParseCommandLineArguments(int argc, char *argv[]);
static ExitCode ValidateUserConfiguration(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.

    // We will receive a SIGTERM if we are shutting down because of a request shutdown or reboot
    // but we also want to preserve any failure exit code from the business logic.
    if (exitCode == ExitCode_Success) {
        exitCode = ExitCode_TermHandler_SigTerm;
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

    ExitCode ec = ValidateUserConfiguration();
    if (ec != ExitCode_Success) {
        return ec;
    }

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Initialize message protocol, UART transport and cloud connection
    ec = MessageProtocol_Initialize(eventLoop, UartTransport_Read, UartTransport_Send);
    if (ec != ExitCode_Success) {
        return ec;
    }

    ec = UartTransport_Initialize(eventLoop, SODAMACHINE_STM32_UART,
                                  MessageProtocol_HandleReceivedMessage);
    if (ec != ExitCode_Success) {
        return ec;
    }

    ec = Cloud_Initialize(eventLoop, (void *)scopeId, BusinessLogic_NotifyFatalError,
                          BusinessLogic_NotifyCloudConnectionChange,
                          BusinessLogic_NotifyCloudFlavorChange);
    if (ec != ExitCode_Success) {
        return ec;
    }

    ec = Update_Initialize(eventLoop, BusinessLogic_NotifyUpdateCheckComplete,
                           BusinessLogic_NotifyUpdateCheckFailed);
    if (ec != ExitCode_Success) {
        return ec;
    }

    ec = BusinessLogic_Initialize(eventLoop);
    if (ec != ExitCode_Success) {
        return ec;
    }

    McuMessaging_Initialize();

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    MessageProtocol_Cleanup();
    UartTransport_Cleanup();
    Cloud_Cleanup();

    EventLoop_Close(eventLoop);
}

/// <summary>
///     Parse the command-line arguments given in the application manifest.
/// </summary>
static void ParseCommandLineArguments(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "ScopeID", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options
    while ((option = getopt_long(argc, argv, "s:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("Warning: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 's':
            scopeId = optarg;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }
}

/// <summary>
///     Validates that the scope ID was set correctly in the app manifest.
/// </summary>
/// <returns>
///     ExitCode_Success if the scope ID was set correctly; otherwise another ExitCode value
///     which indicates the specific failure.
/// </returns>
static ExitCode ValidateUserConfiguration(void)
{
    ExitCode ec = ExitCode_Success;

    if (scopeId == NULL || strcmp(scopeId, "<scopeid>") == 0) {
        ec = ExitCode_Validation_ScopeId;
        Log_Debug(
            "ERROR: Missing scope ID. Please specify the scope ID for your Azure IoT Central app "
            "in the app_manifest.json:\n"
            "    CmdArgs: [ \"--ScopeID\", \"<scopeid>\" ]\n");
    }

    Log_Debug("INFO: Using Azure IoT scope ID: %s\n", scopeId);
    return ec;
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    DebugUart_Init();

    Power_SetPowerSaveMode();

    Log_Debug("ExternalMcuLowPower DeviceToCloud application starting.\n");

    ParseCommandLineArguments(argc, argv);

    exitCode = (sig_atomic_t)InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }

        ExitCode businessLogicExitCode;
        if (BusinessLogic_Run(&businessLogicExitCode)) {
            exitCode = (sig_atomic_t)businessLogicExitCode;
            break;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");

    DebugUart_Cleanup();

    return exitCode;
}
