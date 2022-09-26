# Snippets: Get Azure Sphere Device OS Version

This folder contains one snippet that demonstrates how to get the currently running OS version on the Azure Sphere device for a high-level application.

The snippet uses the [Applications_GetOsVersion](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-applications/function-applications-getosversion) function to get the installed OS version and uses the [Log_Debug](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/function-log-debug) function to display the OS version on an attached device.

### Compile and run the snippet

In order to successfully compile and run the snippet you must
merge the snippet code into your application and call the GetOsVersion function.

### Additional resources

The OS version returned from the Applications_GetOsVersion function is the same value as running the Azure Sphere CLI command [**azsphere device show-os-version**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#show-os-version).
