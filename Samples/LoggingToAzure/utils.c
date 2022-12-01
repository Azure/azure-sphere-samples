/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "utils.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <applibs/log.h>

#include <applibs/wificonfig.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <applibs/networking.h>

#include "exitcodes.h"

#define MAX_IFACE_NAME_LEN 16
#define MAX_MACADDR_OCTETS 6
#define MAX_MACADDR_STR 18
#define MAX_NETWORK_SSID 32

int DateTime_UTC(char *outputBuffer, size_t outputBufferSize, time_t t)
{
    // Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
    static const char *ISO8601Format = "%Y-%m-%dT%H:%M:%SZ";

    struct tm *currentTimeTm;
    currentTimeTm = gmtime(&t);

    size_t size = strftime(outputBuffer, outputBufferSize, ISO8601Format, currentTimeTm);

    if (size == 0) {
        Log_Debug("ERROR: strftime: %s (%d)\n", errno, strerror(errno));
        return -1;
    }

    return (int)size;
}

static struct ifaddrs *GetIface_AddrInfo(char *iface, size_t iface_len, struct ifaddrs *addresses,
                                         int family)
{

    struct ifaddrs *addr = addresses;

    while (addr != NULL) {
        if (addr->ifa_addr) {
            if (strncmp(addr->ifa_name, iface, iface_len) == 0) {
                if (addr->ifa_addr->sa_family == family) {
                    return addr;
                }
            }
        }

        addr = addr->ifa_next;
    }

    return NULL;
}

ExitCode NetIface_IpAddr(char *outBuffer, size_t outBufferLen, char *iface)
{
    if (outBuffer == NULL || iface == NULL)
        return ExitCode_InvalidParameter;

    struct ifaddrs *addresses;
    int ret = ExitCode_InvalidParameter;

    if (getifaddrs(&addresses) == 0) {
        struct ifaddrs *addr =
            GetIface_AddrInfo(iface, strnlen(iface, MAX_IFACE_NAME_LEN), addresses, AF_INET);

        if (addr) {
            struct sockaddr_in *ip_addr = (struct sockaddr_in *)addr->ifa_addr;
            char *ipString = inet_ntoa(ip_addr->sin_addr);
            strncpy(outBuffer, ipString, MIN(strnlen(ipString, 16), outBufferLen));
            ret = ExitCode_Success;
        }
    }

    freeifaddrs(addresses);
    return ret;
}

ExitCode NetIface_MacAddr(char *outBuffer, size_t outBufferLen, char *iface)
{
    if (outBuffer == NULL || iface == NULL)
        return ExitCode_InvalidParameter;

    struct ifaddrs *addresses;
    int ret = ExitCode_InvalidParameter;

    if (getifaddrs(&addresses) == 0) {
        struct ifaddrs *addr =
            GetIface_AddrInfo(iface, strnlen(iface, MAX_IFACE_NAME_LEN), addresses, AF_PACKET);

        if (addr) {
            struct sockaddr_ll *mac_addr = (struct sockaddr_ll *)addr->ifa_addr;
            memcpy(outBuffer, mac_addr->sll_addr, (size_t)MIN(MAX_MACADDR_OCTETS, outBufferLen));
            ret = ExitCode_Success;
        }
    }

    freeifaddrs(addresses);
    return ret;
}

/**
 * @brief An internal function that safely writes formatted strings to a buffer
 */
static void StringBuilderAppend(char *append, char *to, int *currentCount, size_t limit,
                                char *fmtString)
{
    int current = *currentCount;
    if (current > limit)
        return;
    *currentCount += snprintf(to, (size_t)MAX((int)limit - current, 0), fmtString, append);
}

ExitCode NetIfaces_ToString(char *outString, size_t outStringLen, bool report_mac, bool report_ip)
{
    if (outString == NULL || outStringLen == 0)
        return ExitCode_InvalidParameter;

    Networking_NetworkInterface ifaces[NETWORK_REPORT_IFACES_COUNT] = {0};
    int ifaceCount = Networking_GetInterfaces(ifaces, NETWORK_REPORT_IFACES_COUNT);

    if (ifaceCount > NETWORK_REPORT_IFACES_COUNT)
        ifaceCount = NETWORK_REPORT_IFACES_COUNT;

    memset(outString, 0, outStringLen);
    // keep the last bytes as the null terminator
    outStringLen--;

    // keeps track of the number of bytes written so far.
    int outStringIdx = 0;

    for (int i = 0; i < ifaceCount; i++) {
        // interface name
        StringBuilderAppend(ifaces[i].interfaceName, outString + outStringIdx, &outStringIdx,
                            outStringLen, "%s ");

        // Is the interface enabled?
        StringBuilderAppend((char *)((ifaces[i].isEnabled) ? "UP\0" : "DOWN\0"),
                            outString + outStringIdx, &outStringIdx, outStringLen, "%s ");

        // Append SSID info if the interface is wifi...
        if (ifaces[i].ipConfigurationType == Networking_InterfaceMedium_Wifi &&
            ifaces[i].isEnabled) {
            WifiConfig_ConnectedNetwork network;
            WifiConfig_GetCurrentNetwork(&network);

            if (network.ssidLength <= 32) {
                // copy the ssid into a local, null-terminated buffer.
                static char netSSID[MAX_NETWORK_SSID + 1] = {0};
                memset(netSSID, 0, sizeof(netSSID));
                memcpy(netSSID, network.ssid, network.ssidLength);
                netSSID[network.ssidLength] = 0;
                StringBuilderAppend(netSSID, outString + outStringIdx, &outStringIdx, outStringLen,
                                    "%s ");
            }
        }

        // report ip address
        if (report_ip) {
            static char ipString[16];
            memset(ipString, 0, sizeof(ipString));
            int ret = NetIface_IpAddr(ipString, 15, ifaces[i].interfaceName);
            StringBuilderAppend((char *)((ret == ExitCode_Success) ? ipString : "-\0"),
                                outString + outStringIdx, &outStringIdx, outStringLen, "%s ");
        }

        // report mac address.
        if (report_mac) {
            static char macAddr[MAX_MACADDR_OCTETS] = {0};
            int ret = NetIface_MacAddr(macAddr, MAX_MACADDR_OCTETS, ifaces[i].interfaceName);
            if (ret == ExitCode_Success) {
                static char macAddrStr[MAX_MACADDR_STR] = {0};
                memset(macAddrStr, 0, sizeof(macAddrStr));
                snprintf(macAddrStr, MAX_MACADDR_STR, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0],
                         macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
                StringBuilderAppend(macAddrStr, outString + outStringIdx, &outStringIdx,
                                    outStringLen, "%s");
            } else
                StringBuilderAppend((char *)"-\0", outString + outStringIdx, &outStringIdx,
                                    outStringLen, "%s");
        }

        StringBuilderAppend((char *)"\n", outString + outStringIdx, &outStringIdx, outStringLen,
                            "%s");
    }

    return ExitCode_Success;
}

/**
 * @brief An internal function that reverses a string
 *
 * @param s the string to reverse
 * @return ExitCode_Success or ExitCode_InvalidParameter is s is NULL.
 */
static int StringReverse(char *s, size_t outStringLen)
{
    if (s == NULL || outStringLen == 0)
        return ExitCode_InvalidParameter;

    char *j;
    int c;

    j = s + strnlen(s, outStringLen) - 1;

    while (s < j) {
        c = *s;
        *s++ = *j;
        *j-- = (char)c;
    }

    return ExitCode_Success;
}

ExitCode Async_Safe_Itoa(int number, char *outString, size_t outStringLen)
{
    if (outString == NULL || outStringLen == 0)
        return ExitCode_InvalidParameter;

    int i = 0;

    // ensure we keep track of the sign
    int positive = (number >= 0);

    if (positive)
        number = -number;

    // progressivly calculate each character
    // each iteration constrains the range of number to between 0 and 9.
    // numbers are written LSB -> MSB to the string
    do {
        outString[i++] = (char)(abs(number % 10) + (int)'0');
        if (i == outStringLen - 1)
            break;
    } while (abs(number /= 10) > 0);

    // append a negative sign if necessary
    if (!positive && !(i > outStringLen - 2))
        outString[i++] = '-';

    // add the NULL terminator.
    outString[i] = '\0';

    // Numbers are translated LSB first, reverse the final string.
    StringReverse(outString, outStringLen);

    return ExitCode_Success;
}