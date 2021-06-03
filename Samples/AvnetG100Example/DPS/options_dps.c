/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <getopt.h>
#include <stdlib.h>

#include <applibs/log.h>

#include "options.h"
#include "exitcodes.h"
#include "connection_dps.h"

static ExitCode ValidateUserConfiguration(void);

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "The command line arguments for the application shoud be set in app_manifest.json as below:\n"
    "\" CmdArgs \": [\"--ScopeID\", \"<scope_id>\"]\n";

static const char *scopeId = NULL;
static Connection_Dps_Config config = {.scopeId = NULL};

ExitCode Options_ParseArgs(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "ScopeID", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "s:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 's':
            Log_Debug("ScopeID: %s\n", optarg);
            scopeId = optarg;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }

    return ValidateUserConfiguration();
}

void *Options_GetConnectionContext(void)
{
    return (void *)&config;
}

static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (scopeId == NULL) {
        validationExitCode = ExitCode_Validate_ScopeId;
    } else {
        Log_Debug("Using DPS Connection: Azure IoT DPS Scope ID %s\n", scopeId);
        config.scopeId = scopeId;
    }

    if (validationExitCode != ExitCode_Success) {
        Log_Debug(cmdLineArgsUsageText);
    }

    return validationExitCode;
}
