/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// Code Snippet: Automatic DNS

// This code snippet demonstrates how to configure a network interface with
// automatically configured DNS (via DHCP). This is the default behavior and
// is only required if you have previously configured a custom DNS.

// To configure a network interface with automatically configured DNS (via DHCP),
// the application manifest (https://learn.microsoft.com/azure-sphere/app-development/app-manifest)
// must enable the NetworkConfig capability. To enable this capability, copy the
// lines in the Capabilities section of AutomaticDns/app_manifest.json into your
// application manifest file.

#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

static const char networkInterfaceToConfigure[] = "yourNetworkInterface"; // Your network interface.

static int ConfigureNetworkInterfaceWithAutomaticDns(void)
{
    Networking_IpConfig ipConfig;
    Networking_IpConfig_Init(&ipConfig);

    Networking_IpConfig_EnableAutomaticDns(&ipConfig);

    int result = Networking_IpConfig_Apply(networkInterfaceToConfigure, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}