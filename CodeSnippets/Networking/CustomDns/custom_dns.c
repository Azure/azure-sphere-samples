/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// Code Snippet: Custom DNS

// This code snippet demonstrates how to configure a network interface with
// custom DNS servers.

// To configure a network interface with custom DNS servers, the application
// manifest (https://learn.microsoft.com/azure-sphere/app-development/app-manifest)
// must enable the NetworkConfig capability. To enable this capability, copy the
// lines in the Capabilities section of CustomDns/app_manifest.json into your
// application manifest file.

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

static const char networkInterfaceToConfigure[] = "yourNetworkInterface"; // Your network interface.
// A maximum of 3 DNS server addresses can be specified.
static const size_t numOfDnsServerAddressSpecified = 3;
static const char *dnsServerIpAddress[] = {
    "yourDnsServer1", "yourDnsServer2", "yourDnsServer3"}; // Your DNS servers in x.x.x.x notation.

static int ConfigureNetworkInterfaceWithCustomDns(void)
{
    Networking_IpConfig ipConfig;

    // Convert the addresses from the numbers-and-dots notation into integers.
    struct in_addr dnsServers[numOfDnsServerAddressSpecified];
    for (int i = 0; i < numOfDnsServerAddressSpecified; i++) {
        if (inet_pton(AF_INET, dnsServerIpAddress[i], &dnsServers[i]) != 1) {
            Log_Debug("ERROR: Invalid DNS server address or address family specified.\n");
            return -1;
        }
    }

    Networking_IpConfig_Init(&ipConfig);

    int result =
        Networking_IpConfig_EnableCustomDns(&ipConfig, dnsServers, numOfDnsServerAddressSpecified);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_EnableCustomDns: %d (%s)\n", errno, strerror(errno));
        Networking_IpConfig_Destroy(&ipConfig);
        return -1;
    }

    result = Networking_IpConfig_Apply(networkInterfaceToConfigure, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}