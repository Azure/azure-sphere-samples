# Snippets: Memory Overuse Detection And Cleanup

This folder contains one snippet that demonstrates how to detect and handle unexpected memory usage.

The [Memory Overuse Detection and Cleanup snippet](MemoryOveruseDetectionAndCleanup) demonstrates how to detect and gracefully handle unexpected memory usage. Code based on this snippet can be called before or after memory allocations, or both, and can also be called regularly on a timer.

The snippet uses the [Applications_GetTotalMemoryUsageInKB()](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-applications/function-applications-gettotalmemoryusageinkb) function. Similar code could use any of the other functions described [here](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage#determine-run-time-application-ram-usage).

### Compiling and running the snippet

In order to successfully compile and run the snippet, you must replace the placeholder '\<INSERT MEMORY VALUE HERE\>' for the total memory usage limit with an integer value. The limit of the total memory usage should be set based on the application logic and on your expectations of the maximum resources required by the application. For more information on memory usage in high-level Azure Sphere applications, see  [Memory use in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage) and [Best practices for managing RAM usage in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/ram-usage-best-practices).

If the limit is reached, the snippet exits with an [exit code](https://docs.microsoft.com/azure-sphere/app-development/exit-codes) you define. In order to successfully compile and run the snippet, you should declare an ExitCode enum that you can use to define all of your specific exit code values. The ExitCode_Success should always be set to 0, while the ExitCode_CheckTotalMemoryLimit_Overflow and the ExitCode_CheckTotalMemoryLimit_GetTotalMemoryUsageInKB_Failed can be set to any value in the [1, 255] interval.

### Additional logic

If the memory limit is exceeded, the application exits and is restarted by the OS. You may want to insert code to save the application state before exiting; you can then recover the saved state after restart. See [the MutableStorage sample](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/MutableStorage) for an example of how to do so. Constantly writing and erasing the flash eventually wears it out and makes it invalid. Consider saving the state of the application to flash only if the state has changed. The [PowerDown](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Powerdown/main.c) sample provides another example of saving and restoring state.

To aid debugging, telemetry may be sent to the cloud with the memory usage details from the APIs. Sending telemetry can happen before exiting the application or regularly throughout the application's lifetime. For general illustration of how to send telemetry check the [AzureIoT](https://github.com/Azure/azure-sphere-samples/blob/master/Samples/AzureIoT/main.c) sample.

### Output

The memory usage results may vary if the code snippet is run [under the debugger](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage).

### Additional resources

If you want how to detect if an application has a memory leak, you can check the [MemoryUsage tutorial](https://github.com/Azure/azure-sphere-samples/tree/master/Tutorials/MemoryUsage). This tutorial shows how to observe, find, and fix a memory leak using either Visual Studio's [memory usage profiler](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#use-the-visual-studio-performance-profiler) or the Azure Sphere CLI's [**azsphere device app show-memory-stats**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#app-show-memory-stats) command.
