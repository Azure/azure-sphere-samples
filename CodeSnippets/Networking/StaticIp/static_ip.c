// Code Snippet: Static IP

// This code snippet demonstrates how to configure a network interface with 
// a static IP address.

// To configure a network interface with a static IP address, the application 
// manifest (https://docs.microsoft.com/azure-sphere/app-development/app-manifest) 
// must enable the NetworkConfig capability. To enable this capability, copy the
// lines in the Capabilities section of StaticIp/app_manifest.json into your
// application manifest file.

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>

static const char networkInterfaceToConfigure[] = "yourNetworkInterface"; // Your network interface.
static const char staticIpInDotNotation[] = "yourStaticIp"; // Your static IP in x.x.x.x notation.
static const char subnetMaskInDotNotation[] =
    "yourSubnetMask"; // Your subnet mask in x.x.x.x notation.
static const char gatewayIpInDotNotation[] = "yourGatewayIp"; // Your gateway IP in x.x.x.x notation.

static int ConfigureNetworkInterfaceWithStaticIp(void)
{
    struct in_addr staticIpAddress;
    struct in_addr subnetMask;
    struct in_addr gatewayIpAddress;

    Networking_IpConfig ipConfig;

    // Convert the addresses from the numbers-and-dots notation into integers.
    if (inet_pton(AF_INET, staticIpInDotNotation, &staticIpAddress) != 1) {
        Log_Debug("ERROR: Invalid static IP address or address family specified.\n");
        return -1;
    }
    if (inet_pton(AF_INET, subnetMaskInDotNotation, &subnetMask) != 1) {
        Log_Debug("ERROR: Invalid subnet mask or address family specified.\n");
        return -1;
    }
    if (inet_pton(AF_INET, gatewayIpInDotNotation, &gatewayIpAddress) != 1) {
        Log_Debug("ERROR: Invalid gateway IP address or address family specified.\n");
        return -1;
    }

    Networking_IpConfig_Init(&ipConfig);
    Networking_IpConfig_EnableStaticIp(&ipConfig, staticIpAddress, subnetMask, gatewayIpAddress);

    int result = Networking_IpConfig_Apply(networkInterfaceToConfigure, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}