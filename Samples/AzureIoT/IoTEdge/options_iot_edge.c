/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <getopt.h>
#include <stdlib.h>
#include <memory.h>

#include <applibs/log.h>

#include "options.h"
#include "connection_iot_edge.h"

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "The command line arguments for the application shoud be set in app_manifest.json as below:\n"
    "\" CmdArgs \": [\"--Hostname\", \"<iotedgedevice_hostname>\", "
    "\"--IoTEdgeRootCAPath\", \"certs/<iotedgedevice_cert_name>\"]\n";

static char *hostname = NULL; // IoT Edge Hostname.
static char *iotEdgeRootCAPath = NULL;

static ExitCode ValidateUserConfiguration(void);

static Connection_IotEdge_Config config = {.edgeDeviceHostname = NULL, .iotEdgeCACertPath = NULL};

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
ExitCode Options_ParseArgs(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'},
        {.name = "IoTEdgeRootCAPath", .has_arg = required_argument, .flag = NULL, .val = 'i'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "h:i:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 'h':
            Log_Debug("IoT Edge Device Hostname: %s\n", optarg);
            hostname = optarg;
            break;
        case 'i':
            Log_Debug("IoT Edge Root CA Path: %s\n", optarg);
            iotEdgeRootCAPath = optarg;
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

    if (hostname == NULL) {
        validationExitCode = ExitCode_Validate_Hostname;
    }

    if (iotEdgeRootCAPath == NULL) {
        validationExitCode = ExitCode_Validate_IoTEdgeCAPath;
    }

    if (validationExitCode == ExitCode_Success) {
        Log_Debug(
            "Using IoT Edge Connection: IoT Edge device Hostname %s, trusted CA cert path %s\n",
            hostname, iotEdgeRootCAPath);
        config.edgeDeviceHostname = hostname;
        config.iotEdgeCACertPath = iotEdgeRootCAPath;
    }

    if (validationExitCode != ExitCode_Success) {
        Log_Debug(cmdLineArgsUsageText);
    }

    return validationExitCode;
}
