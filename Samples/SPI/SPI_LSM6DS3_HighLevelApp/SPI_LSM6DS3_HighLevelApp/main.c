/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere uses the Azure Sphere SPI APIs to display
// data from an accelerometer connected via SPI.
//
// It uses the APIs for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - SPI (communicates with LSM6DS3 accelerometer)

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
#include <applibs/spi.h>

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed using the project property "Target Hardware Definition Directory".
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
static int spiFd = -1;

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
    // Set bit 7 to instruct the accelerometer that this is a read
    // from register 0x1E.
    static const uint8_t statusRegIdReadCmd = (0x1E | 0x80);
    uint8_t status;
    ssize_t transferredBytes = SPIMaster_WriteThenRead(
        spiFd, &statusRegIdReadCmd, sizeof(statusRegIdReadCmd), &status, sizeof(status));
    if (!CheckTransferSize("SPIMaster_WriteThenRead (STATUS_REG)",
                           sizeof(statusRegIdReadCmd) + sizeof(status), transferredBytes)) {
        terminationRequired = true;
        return;
    }

    if ((status & 0x1) == 0) {
        Log_Debug("INFO: %d: No accelerometer data.\n", iter);
    } else {
        // Read two-byte Z-axis output register.
        // DocID026899 Rev 10, S9.38, OUTZ_L_XL (2Ch)
        // Set bit 7 to instruct the accelerometer that this is a read
        // from register 0x2C.
        static const uint8_t outZLXlReadCmd = (0x2C | 0x80);
        int16_t zRaw;
        transferredBytes = SPIMaster_WriteThenRead(spiFd, &outZLXlReadCmd, sizeof(outZLXlReadCmd),
                                                   (uint8_t *)&zRaw, sizeof(zRaw));
        if (!CheckTransferSize("SPIMaster_WriteThenRead (OUTZ_L_XL)",
                               sizeof(outZLXlReadCmd) + sizeof(zRaw), transferredBytes)) {
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

/// <summary>
///	Demonstrates two ways of reading data from the attached device.
///	This also works as a smoke test to ensure Azure Sphere can talk to the SPI device.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int ReadWhoAmI(void)
{
    // DocID026899 Rev 10, S9.11, WHO_AM_I (0Fh); has fixed value 0x69.
    // Set bit 7 to instruct the accelerometer that this is a read
    // from register 0x0F.
    static const uint8_t whoAmIRegIdReadCmd = (0x0F | 0x80);
    static const uint8_t expectedWhoAmI = 0x69;
    uint8_t actualWhoAmI;

    // Read register value using AppLibs combination read and write API.
    ssize_t transferredBytes =
        SPIMaster_WriteThenRead(spiFd, &whoAmIRegIdReadCmd, sizeof(whoAmIRegIdReadCmd),
                                &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("SPIMaster_WriteThenRead (WHO_AM_I)",
                           sizeof(whoAmIRegIdReadCmd) + sizeof(actualWhoAmI), transferredBytes)) {
        return -1;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (SPIMaster_WriteThenRead)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return -1;
    }

    // Read register value using AppLibs combination read and write API
    static const size_t transferCount = 2;
    SPIMaster_Transfer transfers[transferCount];
    uint8_t actualWhoAmIMultipleTransfers;

    int result = SPIMaster_InitTransfers(transfers, transferCount);
    if (result != 0) {
        return -1;
    }

    transfers[0].flags = SPI_TransferFlags_Write;
    transfers[0].writeData = &whoAmIRegIdReadCmd;
    transfers[0].length = sizeof(whoAmIRegIdReadCmd);

    transfers[1].flags = SPI_TransferFlags_Read;
    transfers[1].readData = &actualWhoAmIMultipleTransfers;
    transfers[1].length = sizeof(actualWhoAmIMultipleTransfers);

    transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);
    if (!CheckTransferSize("SPIMaster_TransferSequential (CTRL3_C)",
                           sizeof(actualWhoAmIMultipleTransfers) + sizeof(whoAmIRegIdReadCmd),
                           transferredBytes)) {
        return -1;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (SPIMaster_TransferSequential)\n",
              actualWhoAmIMultipleTransfers);
    if (actualWhoAmIMultipleTransfers != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return -1;
    }

    // write() then read() does not work for this peripheral. Since that involves two
    // separate driver-level operations, the CS line is deasserted between the write()
    // and read(), and the peripheral loses state about the selected register.
    return 0;
}

/// <summary>
///    Checks the number of transferred bytes for SPI functions and prints an error
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
    const size_t transferCount = 1;
    SPIMaster_Transfer transfer;

    int result = SPIMaster_InitTransfers(&transfer, transferCount);
    if (result != 0) {
        return -1;
    }

    // Reset device to put registers into default state.
    // DocID026899 Rev 10, S9.14, CTRL3_C (12h); [0] = SW_RESET
    static const uint8_t ctrl3cRegId = 0x12;
    const uint8_t resetCommand[] = {ctrl3cRegId, 0x01};

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = resetCommand;
    transfer.length = sizeof(resetCommand);

    ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, &transfer, transferCount);
    if (!CheckTransferSize("SPIMaster_TransferSequential (CTRL3_C)", transfer.length,
                           transferredBytes)) {
        return -1;
    }

    // Set bit 7 to instruct the accelerometer that this is a read
    // from register 0x12.
    static const uint8_t ctrl3cRegIdReadCmd = (0x12 | 0x80);

    // Wait for device to come out of reset.
    uint8_t ctrl3c;
    do {
        transferredBytes = SPIMaster_WriteThenRead(
            spiFd, &ctrl3cRegIdReadCmd, sizeof(ctrl3cRegIdReadCmd), &ctrl3c, sizeof(ctrl3c));
    } while (!(transferredBytes == (sizeof(ctrl3cRegIdReadCmd) + sizeof(ctrl3c)) &&
               (ctrl3c & 0x1) == 0));

    // Use sample range +/- 4g, with 12.5Hz frequency.
    // DocID026899 Rev 10, S9.12, CTRL1_XL (10h)
    static const uint8_t setCtrl1XlCommand[] = {0x10, 0x18};

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = setCtrl1XlCommand;
    transfer.length = sizeof(setCtrl1XlCommand);

    transferredBytes = SPIMaster_TransferSequential(spiFd, &transfer, transferCount);
    if (!CheckTransferSize("SPIMaster_TransferSequential (CTRL1_XL)", transfer.length,
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

    SPIMaster_Config config;
    int ret = SPIMaster_InitConfig(&config);
    if (ret != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
                  errno);
        return -1;
    }
    config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    spiFd = SPIMaster_Open(SAMPLE_LSM6DS3_SPI, SAMPLE_LSM6DS3_SPI_CS, &config);
    if (spiFd < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    int result = SPIMaster_SetBusSpeed(spiFd, 400000);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
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
    CloseFdAndPrintError(spiFd, "Spi");
    CloseFdAndPrintError(accelTimerFd, "accelTimer");
    CloseFdAndPrintError(epollFd, "Epoll");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("SPI accelerometer application starting.\n");
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
