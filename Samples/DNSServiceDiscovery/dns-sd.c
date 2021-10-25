/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dns-sd.h"
#include <applibs/log.h>
#include <errno.h>
#include <resolv.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DNS_SERVER_PORT 53
#define QUERY_BUF_SIZE 2048u
#define ANSWER_BUF_SIZE 2048u
#define DISPLAY_BUF_SIZE 256u

int SendDnsQuery(const char *dName, int class, int type, int fd)
{
    char queryBuf[QUERY_BUF_SIZE];
    if (!dName) {
        Log_Debug("ERROR: Can't send DNS query as the domain name is null.\n");
        errno = EINVAL;
        return -1;
    }

    // Construct the DNS query to send
    int ret = res_init();
    if (ret) {
        Log_Debug("ERROR: res_init: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    int messageSize =
        res_mkquery(ns_o_query, dName, class, type, NULL, 0, NULL, queryBuf, QUERY_BUF_SIZE);
    if (messageSize <= 0) {
        Log_Debug("ERROR: res_mkquery: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Send the constructed DNS query
    struct sockaddr_in si;
    memset(&si, 0, sizeof(si));
    si.sin_family = AF_INET;
    si.sin_port = htons(DNS_SERVER_PORT);
    // NOTE: The Beta support for mDNS currently requires using the loopback IP address as follows.
    // This will most likely be replaced in a future release, causing a breaking change for
    // applications that rely on it.
    si.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ret = sendto(fd, queryBuf, (size_t)messageSize, 0, (struct sockaddr *)&si, sizeof(si));
    if (ret == -1) {
        Log_Debug("ERROR: sendto: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int SendServiceDiscoveryQuery(const char *dName, int fd)
{
    return SendDnsQuery(dName, ns_c_in, ns_t_ptr, fd);
}

int SendServiceInstanceDetailsQuery(const char *instanceName, int fd)
{
    return SendDnsQuery(instanceName, ns_c_in, ns_t_any, fd);
}

int ProcessMessageBySection(char *buf, ssize_t len, ns_msg msg, ns_sect section,
                            ServiceInstanceDetails **instanceDetails)
{
    char displayBuf[DISPLAY_BUF_SIZE];
    ns_rr rr;
    int messageCount = ns_msg_count(msg, section);

    if (!(*instanceDetails)) {
        // Allocate for ServiceInstanceDetails and initialize its members.
        *instanceDetails = malloc(sizeof(ServiceInstanceDetails));
        if (!(*instanceDetails)) {
            Log_Debug("ERROR: recvfrom: %d (%s)\n", errno, strerror(errno));
            return -1;
        }
        (*instanceDetails)->name = NULL;
        (*instanceDetails)->host = NULL;
        (*instanceDetails)->ipv4Address.s_addr = INADDR_NONE;
        (*instanceDetails)->port = 0;
        (*instanceDetails)->txtData = NULL;
        (*instanceDetails)->txtDataLength = 0;
    }

    // Parse each message
    for (int i = 0; i < messageCount; ++i) {
        if (ns_parserr(&msg, section, i, &rr)) {
            Log_Debug("ERROR: ns_parserr: %d (%s)\n", errno, strerror(errno));
            return -1;
        }
        switch (ns_rr_type(rr)) {
        case ns_t_ptr: {
            int compressedNameLength =
                dn_expand(buf, buf + len, ns_rr_rdata(rr), displayBuf, sizeof(displayBuf));
            if (compressedNameLength > 0 && !(*instanceDetails)->name) {
                (*instanceDetails)->name = strdup(displayBuf);
                if (!(*instanceDetails)->name) {
                    Log_Debug("ERROR: strdup for instance name failed: %d (%s)\n", errno,
                              strerror(errno));
                    return -1;
                }
            }
            break;
        }
        case ns_t_srv: {
            // Parse the SRV record and populate the port and host fields in instance details as per
            // DNS SRV record specification: https://tools.ietf.org/rfc/rfc2782.txt
            // SRV record format: Priority|  Weight |   Port  |     Target
            //                   (2 Bytes)|(2 Bytes)|(2 Bytes)|(Remaining Bytes)
            const char *data = ns_rr_rdata(rr);
            int compressedTargetDomainNameLength = dn_expand(
                buf, buf + len, data + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t),
                displayBuf, sizeof(displayBuf));
            if ((*instanceDetails)->port == 0 && compressedTargetDomainNameLength > 0 &&
                !(*instanceDetails)->host) {
                (*instanceDetails)->port =
                    (uint16_t)ns_get16(data + sizeof(uint16_t) + sizeof(uint16_t));
                (*instanceDetails)->host = strdup(displayBuf);
                if (!(*instanceDetails)->host) {
                    Log_Debug("ERROR: strdup: %d (%s)\n", errno, strerror(errno));
                    return -1;
                }
            }
            break;
        }
        case ns_t_txt: {
            // Populate name field in instance details if it hasn't been set
            if (!(*instanceDetails)->name) {
                (*instanceDetails)->name = strdup(ns_rr_name(rr));
                if (!(*instanceDetails)->name) {
                    Log_Debug("ERROR: strdup(ns_rr_name(rr)): %d (%s)\n", errno, strerror(errno));
                    return -1;
                }
            }

            // Get TXT record, populate txtData and txtDataLength fields in instance details
            if (!(*instanceDetails)->txtData) {
                (*instanceDetails)->txtData =
                    malloc(sizeof(unsigned char) * (size_t)(ns_rr_rdlen(rr)));
                if (!(*instanceDetails)->txtData) {
                    Log_Debug("ERROR: malloc: %d (%s)\n", errno, strerror(errno));
                    return -1;
                }
                memcpy((*instanceDetails)->txtData, ns_rr_rdata(rr), ns_rr_rdlen(rr));
                (*instanceDetails)->txtDataLength = ns_rr_rdlen(rr);
            }
            break;
        }
        case ns_t_a: {
            // Get A record (host address), populate ipv4Address field in instance details
            if (ns_rr_rdlen(rr) == sizeof((*instanceDetails)->ipv4Address)) {
                memcpy(&(*instanceDetails)->ipv4Address.s_addr, ns_rr_rdata(rr), ns_rr_rdlen(rr));
            } else {
                Log_Debug("ERROR: Invalid DNS A record length: %d\n", ns_rr_rdlen(rr));
            }
            break;
        }
        default:
            break;
        }
    }
    return 0;
}

int ProcessDnsResponse(int fd, ServiceInstanceDetails **instanceDetails)
{
    char answerBuf[ANSWER_BUF_SIZE];
    ns_msg msg;
    struct sockaddr_in socketAddress;
    socklen_t addrLength = sizeof(socketAddress);
    ssize_t len =
        recvfrom(fd, answerBuf, ANSWER_BUF_SIZE, 0, (struct sockaddr *)&socketAddress, &addrLength);
    if (len == -1) {
        Log_Debug("ERROR: recvfrom: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Check the response has come from the loopback address
    if (socketAddress.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        Log_Debug("ERROR: recvfrom unexpected address: %x\n", socketAddress.sin_addr);
        return -1;
    }

    // Decode received response
    if (ns_initparse(answerBuf, len, &msg) != 0) {
        Log_Debug("ERROR: ns_initparse: %d (%s)\n", errno, strerror(errno));
        goto fail;
    }
    if (ProcessMessageBySection(answerBuf, len, msg, ns_s_an, instanceDetails) != 0) {
        goto fail;
    }
    if (ProcessMessageBySection(answerBuf, len, msg, ns_s_ar, instanceDetails) != 0) {
        goto fail;
    }
    return 0;

fail:
    FreeServiceInstanceDetails(*instanceDetails);
    *instanceDetails = NULL;
    return -1;
}

void FreeServiceInstanceDetails(const ServiceInstanceDetails *details)
{
    if (details) {
        free(details->name);
        free(details->host);
        free(details->txtData);
        free((void *)details);
    }
}