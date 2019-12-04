/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere uses the Azure Sphere I2C APIs to display
// data from an accelerometer connected via I2C.
//
// It uses the APIs for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - i2c (communicates with LSM6DS3 accelerometer)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"

#include <applibs/log.h>
#include <applibs/i2c.h>

// By default, this sample's CMake build targets hardware that follows the MT3620
// Reference Development Board (RDB) specification, such as the MT3620 Dev Kit from
// Seeed Studios.
//
// To target different hardware, you'll need to update the CMake build. The necessary
// steps to do this vary depending on if you are building in Visual Studio, in Visual
// Studio Code or via the command line.
//
// See https://github.com/Azure/azure-sphere-samples/tree/master/Hardware for more details.
//
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

// Support functions.
static void TerminationHandler(int signalNumber);
static void AccelTimerEventHandler(EventData *eventData);
static int ReadWhoAmI(void);
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);
static int InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
static int epollFd = -1;
static int accelTimerFd = -1;
static int i2cFd = -1;

// DocID026899 Rev 10, S6.1.1, I2C operation
// SDO is tied to ground so the least significant bit of the address is zero.
static const uint8_t lsm6ds3Address = 0x6A;

// Termination state
static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Print latest data from accelerometer.
/// </summary>
static void AccelTimerEventHandler(EventData *eventData)
{
    static int iter = 1;

    if (ConsumeTimerFdEvent(accelTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // Status register describes whether accelerometer is available.
    // DocID026899 Rev 10, S9.26, STATUS_REG (1Eh); [0] = XLDA
    static const uint8_t statusRegId = 0x1E;
    uint8_t status;
    ssize_t transferredBytes = I2CMaster_WriteThenRead(
        i2cFd, lsm6ds3Address, &statusRegId, sizeof(statusRegId), &status, sizeof(status));
    if (!CheckTransferSize("I2CMaster_WriteThenRead (STATUS_REG)",
                           sizeof(statusRegId) + sizeof(status), transferredBytes)) {
        terminationRequired = true;
        return;
    }

    if ((status & 0x1) == 0) {
        Log_Debug("INFO: %d: No accelerometer data.\n", iter);
    } else {
        // Read two-byte Z-axis output register.
        // DocID026899 Rev 10, S9.38, OUTZ_L_XL (2Ch)
        static const uint8_t outZLXl = 0x2C;
        int16_t zRaw;
        transferredBytes = I2CMaster_WriteThenRead(i2cFd, lsm6ds3Address, &outZLXl, sizeof(outZLXl),
                                                   (uint8_t *)&zRaw, sizeof(zRaw));
        if (!CheckTransferSize("I2CMaster_WriteThenRead (OUTZ_L_XL)",
                               sizeof(outZLXl) + sizeof(zRaw), transferredBytes)) {
            terminationRequired = true;
            return;
        }

        // DocID026899 Rev 10, S4.1, Mechanical characteristics
        // These constants are specific to LA_So where FS = +/-4g, as set in CTRL1_X.
        double g = (zRaw * 0.122) / 1000.0;
        Log_Debug("INFO: %d: vertical acceleration: %.2lfg\n", iter, g);
    }

    ++iter;
}

// Demonstrates three ways of reading data from the attached device.
// This also works as a smoke test to ensure the Azure Sphere can talk to the I2C device.
static int ReadWhoAmI(void)
{
    // DocID026899 Rev 10, S9.11, WHO_AM_I (0Fh); has fixed value 0x69.
    static const uint8_t whoAmIRegId = 0x0F;
    static const uint8_t expectedWhoAmI = 0x69;
    uint8_t actualWhoAmI;

    // Read register value using AppLibs combination read and write API.
    ssize_t transferredBytes =
        I2CMaster_WriteThenRead(i2cFd, lsm6ds3Address, &whoAmIRegId, sizeof(whoAmIRegId),
                                &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("I2CMaster_WriteThenRead (WHO_AM_I)",
                           sizeof(whoAmIRegId) + sizeof(actualWhoAmI), transferredBytes)) {
        return -1;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (I2CMaster_WriteThenRead)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return -1;
    }

    // Read register value using AppLibs separate read and write APIs.
    transferredBytes = I2CMaster_Write(i2cFd, lsm6ds3Address, &whoAmIRegId, sizeof(whoAmIRegId));
    if (!CheckTransferSize("I2CMaster_Write (WHO_AM_I)", sizeof(whoAmIRegId), transferredBytes)) {
        return -1;
    }
    transferredBytes = I2CMaster_Read(i2cFd, lsm6ds3Address, &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("I2CMaster_Read (WHO_AM_I)", sizeof(actualWhoAmI), transferredBytes)) {
        return -1;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (I2CMaster_Write + I2CMaster_Read)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return -1;
    }

    // Read register value using POSIX APIs.
    // This uses the I2C target address which was set earlier with
    // I2CMaster_SetDefaultTargetAddress.
    transferredBytes = write(i2cFd, &whoAmIRegId, sizeof(whoAmIRegId));
    if (!CheckTransferSize("write (WHO_AM_I)", sizeof(whoAmIRegId), transferredBytes)) {
        return -1;
    }
    transferredBytes = read(i2cFd, &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("read (WHO_AM_I)", sizeof(actualWhoAmI), transferredBytes)) {
        return -1;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (POSIX read + write)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return -1;
    }

    return 0;
}

/// <summary>
///    Checks the number of transferred bytes for I2C functions and prints an error
///    message if the functions failed or if the number of bytes is different than
///    expected number of bytes to be transferred.
/// </summary>
/// <returns>true on success, or false on failure</returns>
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes)
{
    if (actualBytes < 0) {
        Log_Debug("ERROR: %s: errno=%d (%s)\n", desc, errno, strerror(errno));
        return false;
    }

    if (actualBytes != (ssize_t)expectedBytes) {
        Log_Debug("ERROR: %s: transferred %zd bytes; expected %zd\n", desc, actualBytes,
                  expectedBytes);
        return false;
    }

    return true;
}

/// <summary>
///     Resets the accelerometer and samples the vertical acceleration.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int ResetAndSampleLsm6ds3(void)
{
    // Reset device to put registers into default state.
    // DocID026899 Rev 10, S9.14, CTRL3_C (12h); [0] = SW_RESET
    static const uint8_t ctrl3cRegId = 0x12;
    const uint8_t resetCommand[] = {ctrl3cRegId, 0x01};
    ssize_t transferredBytes =
        I2CMaster_Write(i2cFd, lsm6ds3Address, resetCommand, sizeof(resetCommand));
    if (!CheckTransferSize("I2CMaster_Write (CTRL3_C)", sizeof(resetCommand), transferredBytes)) {
        return -1;
    }

    // Wait for device to come out of reset.
    uint8_t ctrl3c;
    do {
        transferredBytes = I2CMaster_WriteThenRead(i2cFd, lsm6ds3Address, &ctrl3cRegId,
                                                   sizeof(ctrl3cRegId), &ctrl3c, sizeof(ctrl3c));
    } while (!(transferredBytes == (sizeof(ctrl3cRegId) + sizeof(ctrl3c)) && (ctrl3c & 0x1) == 0));

    // Use sample range +/- 4g, with 12.5Hz frequency.
    // DocID026899 Rev 10, S9.12, CTRL1_XL (10h)
    static const uint8_t setCtrl1XlCommand[] = {0x10, 0x18};
    transferredBytes =
        I2CMaster_Write(i2cFd, lsm6ds3Address, setCtrl1XlCommand, sizeof(setCtrl1XlCommand));
    if (!CheckTransferSize("I2CMaster_Write (CTRL1_XL)", sizeof(setCtrl1XlCommand),
                           transferredBytes)) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    // Print accelerometer data every second.
    struct timespec accelReadPeriod = {.tv_sec = 1, .tv_nsec = 0};
    // event handler data structures. Only the event handler field needs to be populated.
    static EventData accelEventData = {.eventHandler = &AccelTimerEventHandler};
    accelTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &accelReadPeriod, &accelEventData, EPOLLIN);
    if (accelTimerFd < 0) {
        return -1;
    }

    i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);
    if (i2cFd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    result = I2CMaster_SetTimeout(i2cFd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // This default address is used for POSIX read and write calls.  The AppLibs APIs take a target
    // address argument for each read or write.
    result = I2CMaster_SetDefaultTargetAddress(i2cFd, lsm6ds3Address);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetDefaultTargetAddress: errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    result = ReadWhoAmI();
    if (result != 0) {
        return -1;
    }

    result = ResetAndSampleLsm6ds3();
    if (result != 0) {
        return -1;
    }

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(i2cFd, "i2c");
    CloseFdAndPrintError(accelTimerFd, "accelTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("I2C accelerometer application starting.\n");
    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return 0;
}
