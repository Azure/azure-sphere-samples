# Snippets: Get Azure Sphere Device OS Version

This folder contains one snippet that demonstrates how to get the Azure Sphere Device OS Version in a High-Level application.

The snippet uses the [Applications_GetOsVersion](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-application/function-applications-getosversion) function to get the device OS version and uses Log_Debug to display the OS Version.

### Compiling and running the snippet

In order to successfully compile and run the snippet you must:
* merge the snippet code into your application and call the GetOSVersion function.

### Additional resources

The OS Version returned from Applications_GetOsVersion is the same value as running the Azure Sphere CLI command [azsphere device show-os-version](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#show-os-version).

