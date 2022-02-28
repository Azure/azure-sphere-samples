/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
// Code Snippet: Read ADC channel via sysfs nodes on Raspberry Pi

// This code snippet demonstrates how to read a value from the MCP3008 ADC chip (which is connected
// to a Raspberry Pi 4 Model B) using Linux sysfs nodes and displays the value in volts.

// Refer to LinuxSysfsNodes/README.md for prerequisites and circuit information.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int OpenAdc(int adcChannel);
static int ReadAdc(int adcFd, int *outSampleValue);
static int ReadAdcChannel(void);

static const char iioSysPath[] = "/sys/bus/iio/devices/iio:device0/";
static const int channelNumber = 0; // Channel 0.
static const int sampleBitCount = 10;
static const float referenceVoltage = 3.3f; // Vref is 3.3V.

/// <summary>
/// Reads the value from the ADC channel and displays the value in volts.
/// </summary>
/// <returns>0 on success, else -1.</returns>
static int ReadAdcChannel(void)
{
    int adcFd = -1;
    int sampleValue = 0;
    int retVal = -1;

    adcFd = OpenAdc(channelNumber);
    if (adcFd == -1) {
        goto cleanup;
    }

    if (ReadAdc(adcFd, &sampleValue) == -1) {
        goto cleanup;
    }

    // Calculate voltage.
    float maxSample = (float)((1 << sampleBitCount) - 1);
    float voltage = ((float)sampleValue * referenceVoltage) / maxSample;
    printf("The out sample value is %.3f V.\n", voltage);
    retVal = 0;

cleanup:
    // Close the file descriptor.
    if (adcFd >= 0) {
        int result = close(adcFd);
        if (result != 0) {
            fprintf(stderr, "ERROR: Could not close ADC fd: %s (%d).\n", strerror(errno), errno);
        }
    }

    return retVal;
}

/// <summary>
/// Helper function to open the ADC file descriptor.
/// </summary>
/// <param name="adcChannel">Channel number to open.</param>
/// <returns>Value of fd if succesful, -1 on error</returns>
static int OpenAdc(int adcChannel)
{
    // Format the path for the channel.
    char path[128];
    int len = snprintf(path, sizeof(path), "%sin_voltage%d_raw", iioSysPath, adcChannel);
    if (len < 0 || len >= sizeof(path)) {
        fprintf(stderr, "ERROR: Cannot format ADC path to buffer.\n");
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "ERROR: OpenAdc failed with error: %s (%d) for path %s.\n", strerror(errno),
                errno,
               path);
        return -1;
    }

    return fd;
}

/// <summary>
/// Helper function which reads the specified ADC channel.
/// </summary>
/// <param name="adcFd">ADC file descriptor.</param>
/// <param name="outSampleValue">Holds the value read from the channel.</param>
/// <returns>0 on success, else -1</returns>
static int ReadAdc(int adcFd, int *outSampleValue)
{
    if (adcFd < 0) {
        fprintf(stderr, "ERROR: Invalid file descriptor.\n");
        return -1;
    }

    // Buffer to hold decimal representation of 4-byte integer value and null terminator.
    char dataBuffer[12];
    memset(dataBuffer, 0, sizeof(dataBuffer));

    if (read(adcFd, dataBuffer, sizeof(dataBuffer)) == -1) {
        fprintf(stderr, "ERROR: ReadAdc failed with error: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    *outSampleValue = atoi(dataBuffer);

    return 0;
}