/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/**
 * Contains useful utility function/macro definitions.
 */

#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "exitcodes.h"

// macros for MAX/MIN of a number
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// the expected buffer size to convert a time to UTC format.
#define DATETIME_BUFFER_SIZE 128
// the expected buffer size to retrieve the full device id.
#define DEVICE_ID_BUFFER_SIZE 134

// the maximum number of interfaces that will be reported by
// NetIfaces_ToString.
#ifndef NETWORK_REPORT_IFACES_COUNT
#define NETWORK_REPORT_IFACES_COUNT 5
#endif

/// <summary>
///      Formats a given time stuct as a UTC date time string in the provided buffer.
///      The recommended length for outputBuffer is DATETIME_BUFFER_SIZE
/// </summary>
/// <param name="outputBuffer">The buffer to store the string in</param>
/// <returns>The length of the string written to outputBuffer</returns>
int DateTime_UTC(char *outputBuffer, size_t outputBufferSize, time_t t);

/// <summary>
///      Retrieves the IP address for the given interface.
///      The recommended length for the outputBuffer is 12.
///
///      Note: the written string may not be null terminated if the provided buffer length
///            is too small.
/// </summary>
/// <param name="outputBuffer">The buffer to store the resulting IP Address in</param>
/// <param name="outputBufferLen">The length of the output buffer</param>
/// <param name="iface">a null terminated string containing the name of the interface to fetch (e.g.
/// wlan0\0)</param> <returns>ExitCode_Success or ExitCode_InvalidParameter</returns>
ExitCode NetIface_IpAddr(char *outBuffer, size_t outBufferLen, char *iface);

/// <summary>
///      Retrieves the MAC address for the given interface.
///      The recommended length for the outputBuffer is 6.
///
///      Note: the characters written to outBuffer are the raw MAC address octects. Octets are not
///      null-terminated.
/// </summary>
/// <param name="outputBuffer">The buffer to store the resulting IP Address in</param>
/// <param name="outputBufferLen">The length of the output buffer</param>
/// <param name="iface">a null terminated string containing the name of the interface to fetch (e.g.
/// wlan0\0)</param> <returns>ExitCode_Success or ExitCode_InvalidParameter</returns>
ExitCode NetIface_MacAddr(char *outBuffer, size_t outBufferLen, char *iface);

/// <summary>
///      Generates a string describing the available network interfaces
///      The maximum number of interfaces used is constrained by the value of
///      NETWORK_REPORT_IFACES_COUNT.
/// </summary>
/// <param name="outputBuffer">The buffer to store the resulting IP Address in</param>
/// <param name="outputBufferLen">The length of the output buffer</param>
/// <param name="mac">if true, write the MAC address to outputBuffer</param>
/// <param name="ip">if true, write the IP address to outputBuffer</param>
/// <returns>ExitCode_Success or ExitCode_InvalidParameter</returns>
ExitCode NetIfaces_ToString(char *outBuffer, size_t outBufferLen, bool mac, bool ip);

/// <summary>
///      Safely converts a number to string without the use of format strings.
/// </summary>
/// <param name="number">The buffer to store the resulting IP Address in</param>
/// <param name="outString">The buffer where the converted integer will be stored. Recommended
/// buffer size is 12 (to cover the range -2147483648 to 2147483647).</param> <param
/// name="outStringLen">The length of the output buffer.</param> <returns>ExitCode_Success or
/// ExitCode_InvalidParameter</returns>
ExitCode Async_Safe_Itoa(int number, char *outString, size_t outStringLen);