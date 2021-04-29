// Code Snippet: Memory Overuse Detection And Cleanup

// This code snippet demonstrates how to detect and gracefully handle unexpected memory usage,
// e.g. due to leak. The code based on this snippet might be called before and/or after memory
// allocations, and also regularly on a timer.

// Because the application will be restarted by the OS if the memory limit was exceeded, its
// state may be saved before exiting and recovered afterwards. For illustration of how to save
// and recover the state of the application check the MutableStorage and the PowerDown samples:
// https://github.com/Azure/azure-sphere-samples/tree/master/Samples/MutableStorage/main.c
// https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Powerdown/main.c

#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/applications.h>

// Set this limit based on the expectation of the maximum resources required by the application
static const size_t TotalMemoryLimit = <INSERT MEMORY VALUE HERE>; // Change this value according to the app constraints

static ExitCode CheckTotalMemoryLimit(void)
{
    // Depending on the logic of the application, the call to the
    // Applications_GetTotalMemoryUsageInKB may be replaced with any of the functions described here
    // https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#determine-run-time-application-ram-usage
    size_t totalMemoryUsage = Applications_GetTotalMemoryUsageInKB();

    if (totalMemoryUsage == 0) {
        Log_Debug("ERROR: Applications_GetTotalMemoryUsageInKB failed: %s (%d)\n", strerror(errno),
                  errno);

        // User defined: https://docs.microsoft.com/azure-sphere/app-development/exit-codes
        return ExitCode_CheckTotalMemoryLimit_GetTotalMemoryUsageInKB_Failed;
    }

    // To aid debugging, telemetry may be sent to the cloud with the memory usage details from
    // the memory APIs. For general illustration of how to send telemetry check the AzureIoT sample:
    // https://github.com/Azure/azure-sphere-samples/blob/master/Samples/AzureIoT/main.c
    // e.g SendTelemetry("{\"TotalMemoryUsed\" : totalMemoryUsage}");
    if (totalMemoryUsage >= TotalMemoryLimit) {
        Log_Debug("ERROR: TotalMemoryUsed reached: %zu KB\n", totalMemoryUsage);

        // User defined: https://docs.microsoft.com/azure-sphere/app-development/exit-codes
        return ExitCode_CheckTotalMemoryLimit_Overflow;
    }

    // User defined: https://docs.microsoft.com/azure-sphere/app-development/exit-codes
    return ExitCode_Success;
}
