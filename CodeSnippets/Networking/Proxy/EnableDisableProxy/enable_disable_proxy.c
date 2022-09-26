/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// Code Snippet : Enable/Disable Configured Proxy

// This code snippet demonstrates how to enable/disable an already configured proxy on
// an Azure Sphere device.

// To enable/disable the proxy, the application manifest
// (https://learn.microsoft.com/azure-sphere/app-development/app-manifest)
// must enable the NetworkConfig capability. To enable this capability, copy the
// lines in the Capabilities section of EnableDisableProxy/app_manifest.json into
// your application manifest file.

#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

/// <summary>
/// Enable or disable an already configured proxy.
/// </summary>
/// <param name="enableProxy">To enable or disable proxy. Set to true to enable, and false to
/// disable proxy</param>
/// <returns>0 if successful, -1 on error</returns>
static int EnableDisableProxy(bool enableProxy)
{
    int result = -1;
    Networking_ProxyConfig  *proxyConfig = Networking_Proxy_Create();
    if (proxyConfig == NULL) {
        Log_Debug("ERROR: Networking_Proxy_Create(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    // Need to get the current config, otherwise the existing config will get overwritten with a
    // blank/default config when the change is applied.
    if (Networking_Proxy_Get(proxyConfig) == -1) {

        if (errno == ENOENT) {
            Log_Debug("There is currently no proxy configured.\n");
        } else {
            Log_Debug("ERROR: Networking_Proxy_Get(): %d (%s)\n", errno, strerror(errno));
        }
        goto cleanup;
    }

    // Get the proxy options.
    Networking_ProxyOptions proxyOptions = 0;
    if (Networking_Proxy_GetProxyOptions(proxyConfig, &proxyOptions) == -1) {
        Log_Debug("ERROR: Networking_Proxy_GetProxyOptions(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    // Set the enabled/disabled proxy option.
    if (enableProxy) {
        // Set flag Networking_ProxyOptions_Enabled;
        proxyOptions |= Networking_ProxyOptions_Enabled;
    } else {
        // Reset flag Networking_ProxyOptions_Enabled;
        proxyOptions &= ~((unsigned int)Networking_ProxyOptions_Enabled);
    }

    if (Networking_Proxy_SetProxyOptions(proxyConfig, proxyOptions) == -1) {
        Log_Debug("ERROR: Networking_Proxy_SetProxyOptions(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
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
