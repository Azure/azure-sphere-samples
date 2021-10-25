// Code Snippet : Configure Proxy Settings

// This code snippet demonstrates how to configure the proxy settings on an Azure Sphere device.

// To configure the proxy settings, the application manifest
// (https://docs.microsoft.com/azure-sphere/app-development/app-manifest)
// must enable the NetworkConfig capability. To enable this capability, copy the
// lines in the Capabilities section of SetProxySettings/app_manifest.json into
// your application manifest file.

#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

static const char *proxyAddress = NULL;     // Placeholder for your proxy address.
static const uint16_t proxyPort = 0;        // Placeholder for your proxy port.
static const char *proxyUsername = NULL;    // Placeholder for username to use with basic
                                            // authentication, or set to NULL to use anonymous
                                            // authentication.
static const char *proxyPassword = NULL;    // Placeholder for password to use with basic
                                            // authentication, or set to NULL to use anonymous
                                            // authentication.
static const char *noProxyAddresses = NULL; // Placeholder for your your comma-separated list of
                                            // host addresses for which proxy should not be used.
                                            // Format is "hostAddress1,hostAddress2,hostAddressN".
                                            // This is an optional configuration.

static int ConfigureProxySettings(void)
{
    int result = -1;

    // By default, proxy configuration option Networking_ProxyOptions_Enabled is set and the proxy
    // type is Networking_ProxyType_HTTP.
    Networking_ProxyConfig *proxyConfig = Networking_Proxy_Create();
    if (proxyConfig == NULL) {
        Log_Debug("ERROR: Networking_Proxy_Create(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    // Set the proxy address and port.
    if (Networking_Proxy_SetProxyAddress(proxyConfig, proxyAddress, proxyPort) == -1) {
        Log_Debug("ERROR: Networking_Proxy_SetProxyAddress(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    // If both username and password are set, use basic authentication. Otherwise use anonymous
    // authentication.
    if ((proxyUsername != NULL) && (proxyPassword != NULL)) {
        if (Networking_Proxy_SetBasicAuthentication(proxyConfig, proxyUsername, proxyPassword) ==
            -1) {
            Log_Debug("ERROR: Networking_Proxy_SetBasicAuthentication(): %d (%s)\n", errno,
                      strerror(errno));
            goto cleanup;
        }
    } else {
        if (Networking_Proxy_SetAnonymousAuthentication(proxyConfig) == -1) {
            Log_Debug("ERROR: Networking_Proxy_SetAnonymousAuthentication(): %d (%s)\n", errno,
                      strerror(errno));
            goto cleanup;
        }
    }

    // Set addresses for which proxy should not be used if "noProxyAddresses" is modified.
    if (noProxyAddresses != NULL) {
        if (Networking_Proxy_SetProxyNoProxyAddresses(proxyConfig, noProxyAddresses) == -1) {
            Log_Debug("ERROR: Networking_Proxy_SetProxyNoProxyAddresses(): %d (%s)\n", errno,
                      strerror(errno));
            goto cleanup;
        }
    }

    // Apply the proxy configuration.
    if (Networking_Proxy_Apply(proxyConfig) == -1) {
        Log_Debug("ERROR: Networking_Proxy_Apply(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    result = 0;

cleanup:
    if (proxyConfig) {
        Networking_Proxy_Destroy(proxyConfig);
    }
    return result;
}
