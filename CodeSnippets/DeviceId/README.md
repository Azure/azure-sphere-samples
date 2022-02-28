# Snippets: Get Azure Sphere Device ID

This folder contains one snippet that demonstrates how to get the Azure Sphere Device ID in a High-Level application.

The snippet uses the [Application_IsDeviceAuthReady](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-application/function-application-isdeviceauthready) function to verify that the device authentication and attestation (DAA) certificate for the device is ready. The snippet also uses the [DeviceAuth_GetCertificatePath](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/tlsutils/function-deviceauth-getcertificatepath) function to the file path to a client certificate managed by the Azure Sphere OS.

### Compiling and running the snippet

In order to successfully compile and run the snippet you must:
* merge the [app_manifest.json](https://docs.microsoft.com/azure-sphere/app-development/app-manifest) file with your application
* modify the `DeviceAuthentication` UUID to match the tenant ID that the device is claimed into.
* modify your [CMakeLists.txt](https://docs.microsoft.com/azure-sphere/app-development/using-cmake-functions) - add `wolfssl tlsutils` to `target_link_libraries`

### Additional resources

More information on 'Device Identity' can be found [here](https://docs.microsoft.com/azure-sphere/deployment/device-identity)