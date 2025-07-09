/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere uses the Azure Sphere SPI APIs to display
// data from an accelerometer connected via SPI.
//
// It uses the APIs for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - SPI (communicates with LSM6DS3 accelerometer)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"

#include <applibs/log.h>
#include <applibs/spi.h>
#include <applibs/eventloop.h>

// The following #include imports a "sample appliance" hardware definition. This provides a set of
// named constants such as SAMPLE_BUTTON_1 which are used when opening the peripherals, rather
// that using the underlying pin names. This enables the same code to target different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio. To target different hardware, you'll
// need to update the TARGET_HARDWARE variable in CMakeLists.txt - see instructions in that file.
//
// You can also use hardware definitions related to all other peripherals on your dev board because
// the sample_appliance header file recursively includes underlying hardware definition headers.
// See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
#include <hw/sample_appliance.h>

#include "eventloop_timer_utilities.h"

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_AccelTimerHandler_Consume = 2,
    ExitCode_AccelTimerHandler_ReadStatus = 3,
    ExitCode_AccelTimerHandler_ReadZAcceleration = 4,

    ExitCode_ReadWhoAmI_WriteThenRead = 5,
    ExitCode_ReadWhoAmI_WriteThenReadWrongWhoAmI = 6,
    ExitCode_ReadWhoAmI_InitTransfers = 7,
    ExitCode_ReadWhoAmI_TransferSequential = 8,
    ExitCode_ReadWhoAmI_TransferSequentialWrongWhoAmI = 9,

    ExitCode_Reset_InitTransfers = 10,
    ExitCode_Reset_TransferSequentialReset = 11,
    ExitCode_Reset_TransferSequentialSetRange = 12,

    ExitCode_Init_EventLoop = 13,
    ExitCode_Init_AccelTimer = 14,
    ExitCode_Init_InitConfig = 15,
    ExitCode_Init_OpenSpiMaster = 16,
    ExitCode_Init_SetBusSpeed = 17,
    ExitCode_Init_SetMode = 18,

    ExitCode_Main_EventLoopFail = 19
} ExitCode;

// Support functions.
static void TerminationHandler(int signalNumber);
static void AccelTimerEventHandler(EventLoopTimer *timer);
static ExitCode ReadWhoAmI(void);
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
static int spiFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *accelTimer = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Print latest data from accelerometer.
/// </summary>
static void AccelTimerEventHandler(EventLoopTimer *timer)
{
    static int iter = 1;

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TermHandler_SigTerm;
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
        exitCode = ExitCode_AccelTimerHandler_ReadStatus;
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
            exitCode = ExitCode_AccelTimerHandler_ReadZAcceleration;
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
///     Demonstrates two ways of reading data from the attached device.
//      This also works as a smoke test to ensure the Azure Sphere device can talk to
///     the SPI device.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ReadWhoAmI(void)
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
        return ExitCode_ReadWhoAmI_WriteThenRead;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (SPIMaster_WriteThenRead)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return ExitCode_ReadWhoAmI_WriteThenReadWrongWhoAmI;
    }

    // Read register value using AppLibs combination read and write API
    static const size_t transferCount = 2;
    SPIMaster_Transfer transfers[transferCount];
    uint8_t actualWhoAmIMultipleTransfers;

    int result = SPIMaster_InitTransfers(transfers, transferCount);
    if (result != 0) {
        return ExitCode_ReadWhoAmI_InitTransfers;
    }

    transfers[0].flags = SPI_TransferFlags_Write;
    transfers[0].writeData = &whoAmIRegIdReadCmd;
    transfers[0].length = sizeof(whoAmIRegIdReadCmd);

    transfers[1].flags = SPI_TransferFlags_Read;
    transfers[1].readData = &actualWhoAmIMultipleTransfers;
    transfers[1].length = sizeof(actualWhoAmIMultipleTransfers);

    transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);
    if (!CheckTransferSize("SPIMaster_TransferSequential (WHO_AM_I)",
                           sizeof(actualWhoAmIMultipleTransfers) + sizeof(whoAmIRegIdReadCmd),
                           transferredBytes)) {
        return ExitCode_ReadWhoAmI_TransferSequential;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (SPIMaster_TransferSequential)\n",
              actualWhoAmIMultipleTransfers);
    if (actualWhoAmIMultipleTransfers != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return ExitCode_ReadWhoAmI_TransferSequentialWrongWhoAmI;
    }

    // write() then read() does not work for this peripheral. Since that involves two
    // separate driver-level operations, the CS line is deasserted between the write()
    // and read(), and the peripheral loses state about the selected register.
    return ExitCode_Success;
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
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ResetAndSetSampleRange(void)
{
    const size_t transferCount = 1;
    SPIMaster_Transfer transfer;

    int result = SPIMaster_InitTransfers(&transfer, transferCount);
    if (result != 0) {
        return ExitCode_Reset_InitTransfers;
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
        return ExitCode_Reset_TransferSequentialReset;
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
        return ExitCode_Reset_TransferSequentialSetRange;
    }

    return 0;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Print accelerometer data every second.
    struct timespec accelReadPeriod = {.tv_sec = 1, .tv_nsec = 0};
    accelTimer = CreateEventLoopPeriodicTimer(eventLoop, &AccelTimerEventHandler, &accelReadPeriod);
    if (accelTimer == NULL) {
        return ExitCode_Init_AccelTimer;
    }

    SPIMaster_Config config;
    int ret = SPIMaster_InitConfig(&config);
    if (ret != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
                  errno);
        return ExitCode_Init_InitConfig;
    }
    config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    spiFd = SPIMaster_Open(SAMPLE_LSM6DS3_SPI, SAMPLE_LSM6DS3_SPI_CS, &config);
    if (spiFd == -1) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_OpenSpiMaster;
    }

    int result = SPIMaster_SetBusSpeed(spiFd, 400000);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetBusSpeed;
    }

    result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetMode;
    }

    ExitCode localExitCode = ReadWhoAmI();
    if (localExitCode == ExitCode_Success) {
        localExitCode = ResetAndSetSampleRange();
    }

    return localExitCode;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(accelTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(spiFd, "Spi");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("SPI accelerometer application starting.\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}
