// Code Snippet : Get Proxy Settings

// This code snippet demonstrates how to retrieve the proxy settings on an Azure Sphere device.

// To retrieve the proxy settings, the application manifest
// (https://docs.microsoft.com/azure-sphere/app-development/app-manifest)
// must enable the either ReadNetworkProxyConfig or NetworkConfig capability. To enable this
// capability, copy the lines in the Capabilities section of GetProxySettings/app_manifest.json
// into your application manifest file.

#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

static int GetProxySettings(void)
{
    int result = -1;
    Networking_ProxyConfig *proxyConfig = Networking_Proxy_Create();
    if (proxyConfig == NULL) {
        Log_Debug("ERROR: Networking_Proxy_Create(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    if (Networking_Proxy_Get(proxyConfig) == -1) {
        if (errno == ENOENT) {
            Log_Debug("ERROR: There is currently no proxy configured.\n");
        } else {
            Log_Debug("ERROR: Networking_Proxy_Get(): %d (%s)\n", errno, strerror(errno));
        }
        goto cleanup;
    }

    // Proxy address.
    const char *proxyAddress = Networking_Proxy_GetProxyAddress(proxyConfig);
    if (proxyAddress == NULL) {
        Log_Debug("ERROR: Networking_Proxy_GetProxyAddress(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    Log_Debug("Proxy Address: %s\n", proxyAddress);

    // Proxy port.
    uint16_t proxyPort;
    if (Networking_Proxy_GetProxyPort(proxyConfig, &proxyPort) == -1) {
        Log_Debug("ERROR: Networking_Proxy_Get(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    Log_Debug("Proxy Port: %u\n", proxyPort);

    // Proxy type.
    Networking_ProxyType proxyType = Networking_Proxy_GetProxyType(proxyConfig);
    if (proxyType == Networking_ProxyType_Invalid) {
        Log_Debug("ERROR: Networking_Proxy_GetProxyType(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    Log_Debug("Proxy Type: %s\n", (proxyType == 0) ? "HTTP" : "Invalid");

    // Proxy auth type.
    Networking_ProxyAuthType proxyAuthType = Networking_Proxy_GetAuthType(proxyConfig);
    if (proxyAuthType == Networking_ProxyAuthType_Invalid) {
        Log_Debug("ERROR: Networking_Proxy_GetAuthType(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    char *authType = "Invalid";
    if (proxyAuthType == Networking_ProxyAuthType_Anonymous) {
        authType = "Anonymous";
    } else if (proxyAuthType == Networking_ProxyAuthType_Basic) {
        authType = "Basic";
    }
    Log_Debug("Proxy Auth Type: %s\n", authType);

    if (proxyAuthType == Networking_ProxyAuthType_Basic) {
        // Proxy username.
        const char *proxyUsername = Networking_Proxy_GetProxyUsername(proxyConfig);
        if (proxyUsername == NULL) {
            Log_Debug("ERROR: Networking_Proxy_GetProxyUsername() returned NULL\n");
            goto cleanup;
        }
        Log_Debug("Proxy Username: %s\n", proxyUsername);

        // Proxy password.
        const char *proxyPassword = Networking_Proxy_GetProxyPassword(proxyConfig);
        if (proxyPassword == NULL) {
            Log_Debug("ERROR: Networking_Proxy_GetProxyPassword() returned NULL\n");
            goto cleanup;
        }
        Log_Debug("Proxy Password: %s\n", proxyPassword);
    }

    // Comma-separated string of addresses for which proxy should not be used.
    const char *noProxyAddresses = Networking_Proxy_GetNoProxyAddresses(proxyConfig);
    if (noProxyAddresses == NULL) {
        Log_Debug("ERROR: Networking_Proxy_GetNoProxyAddresses() returned NULL\n");
        goto cleanup;
    }
    Log_Debug("No Proxy Addresses: %s\n", noProxyAddresses);

    // Proxy options.
    Networking_ProxyOptions proxyOptions = 0;
    if (Networking_Proxy_GetProxyOptions(proxyConfig, &proxyOptions) == -1) {
        Log_Debug("ERROR: Networking_Proxy_GetProxyOptions(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    Log_Debug("Proxy Options: %s\n",
              (proxyOptions & Networking_ProxyOptions_Enabled) ? "Enabled" : "Disabled");

    result = 0;

cleanup:
    if (proxyConfig) {
        Networking_Proxy_Destroy(proxyConfig);
    }
    return result;
}
