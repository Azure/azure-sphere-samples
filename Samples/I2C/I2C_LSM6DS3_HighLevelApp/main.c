/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere uses the Azure Sphere I2C APIs to display
// data from an accelerometer connected via I2C.
//
// It uses the APIs for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - i2c (communicates with LSM6DS3 accelerometer)
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
#include <applibs/i2c.h>
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
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_AccelTimer_Consume = 2,
    ExitCode_AccelTimer_ReadStatus = 3,
    ExitCode_AccelTimer_ReadZAccel = 4,

    ExitCode_ReadWhoAmI_WriteThenRead = 5,
    ExitCode_ReadWhoAmI_WriteThenReadCompare = 6,
    ExitCode_ReadWhoAmI_Write = 7,
    ExitCode_ReadWhoAmI_Read = 8,
    ExitCode_ReadWhoAmI_WriteReadCompare = 9,
    ExitCode_ReadWhoAmI_PosixWrite = 10,
    ExitCode_ReadWhoAmI_PosixRead = 11,
    ExitCode_ReadWhoAmI_PosixCompare = 12,

    ExitCode_SampleRange_Reset = 13,
    ExitCode_SampleRange_SetRange = 14,

    ExitCode_Init_EventLoop = 15,
    ExitCode_Init_AccelTimer = 16,
    ExitCode_Init_OpenMaster = 17,
    ExitCode_Init_SetBusSpeed = 18,
    ExitCode_Init_SetTimeout = 19,
    ExitCode_Init_SetDefaultTarget = 20,

    ExitCode_Main_EventLoopFail = 21
} ExitCode;

// Support functions.
static void TerminationHandler(int signalNumber);
static void AccelTimerEventHandler(EventLoopTimer *timer);
static ExitCode ReadWhoAmI(void);
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);
static ExitCode ResetAndSetSampleRange(void);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
static int i2cFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *accelTimer = NULL;

// DocID026899 Rev 10, S6.1.1, I2C operation
// SDO is tied to ground so the least significant bit of the address is zero.
static const uint8_t lsm6ds3Address = 0x6A;

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
        exitCode = ExitCode_AccelTimer_Consume;
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
        exitCode = ExitCode_AccelTimer_ReadStatus;
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
            exitCode = ExitCode_AccelTimer_ReadZAccel;
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
///     Demonstrates three ways of reading data from the attached device.
//      This also works as a smoke test to ensure the Azure Sphere device can talk to
///     the I2C device.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ReadWhoAmI(void)
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
        return ExitCode_ReadWhoAmI_WriteThenRead;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (I2CMaster_WriteThenRead)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return ExitCode_ReadWhoAmI_WriteThenReadCompare;
    }

    // Read register value using AppLibs separate read and write APIs.
    transferredBytes = I2CMaster_Write(i2cFd, lsm6ds3Address, &whoAmIRegId, sizeof(whoAmIRegId));
    if (!CheckTransferSize("I2CMaster_Write (WHO_AM_I)", sizeof(whoAmIRegId), transferredBytes)) {
        return ExitCode_ReadWhoAmI_Write;
    }
    transferredBytes = I2CMaster_Read(i2cFd, lsm6ds3Address, &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("I2CMaster_Read (WHO_AM_I)", sizeof(actualWhoAmI), transferredBytes)) {
        return ExitCode_ReadWhoAmI_Read;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (I2CMaster_Write + I2CMaster_Read)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return ExitCode_ReadWhoAmI_WriteReadCompare;
    }

    // Read register value using POSIX APIs.
    // This uses the I2C target address which was set earlier with
    // I2CMaster_SetDefaultTargetAddress.
    transferredBytes = write(i2cFd, &whoAmIRegId, sizeof(whoAmIRegId));
    if (!CheckTransferSize("write (WHO_AM_I)", sizeof(whoAmIRegId), transferredBytes)) {
        return ExitCode_ReadWhoAmI_PosixWrite;
    }
    transferredBytes = read(i2cFd, &actualWhoAmI, sizeof(actualWhoAmI));
    if (!CheckTransferSize("read (WHO_AM_I)", sizeof(actualWhoAmI), transferredBytes)) {
        return ExitCode_ReadWhoAmI_PosixRead;
    }
    Log_Debug("INFO: WHO_AM_I=0x%02x (POSIX read + write)\n", actualWhoAmI);
    if (actualWhoAmI != expectedWhoAmI) {
        Log_Debug("ERROR: Unexpected WHO_AM_I value.\n");
        return ExitCode_ReadWhoAmI_PosixCompare;
    }

    return ExitCode_Success;
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
///     Resets the accelerometer and sets the sample range.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ResetAndSetSampleRange(void)
{
    // Reset device to put registers into default state.
    // DocID026899 Rev 10, S9.14, CTRL3_C (12h); [0] = SW_RESET
    static const uint8_t ctrl3cRegId = 0x12;
    const uint8_t resetCommand[] = {ctrl3cRegId, 0x01};
    ssize_t transferredBytes =
        I2CMaster_Write(i2cFd, lsm6ds3Address, resetCommand, sizeof(resetCommand));
    if (!CheckTransferSize("I2CMaster_Write (CTRL3_C)", sizeof(resetCommand), transferredBytes)) {
        return ExitCode_SampleRange_Reset;
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
        return ExitCode_SampleRange_SetRange;
    }

    return ExitCode_Success;
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
    static const struct timespec accelReadPeriod = {.tv_sec = 1, .tv_nsec = 0};
    accelTimer = CreateEventLoopPeriodicTimer(eventLoop, &AccelTimerEventHandler, &accelReadPeriod);
    if (accelTimer == NULL) {
        return ExitCode_Init_AccelTimer;
    }

    i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);
    if (i2cFd == -1) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_OpenMaster;
    }

    int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetBusSpeed;
    }

    result = I2CMaster_SetTimeout(i2cFd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetTimeout;
    }

    // This default address is used for POSIX read and write calls.  The AppLibs APIs take a target
    // address argument for each read or write.
    result = I2CMaster_SetDefaultTargetAddress(i2cFd, lsm6ds3Address);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetDefaultTargetAddress: errno=%d (%s)\n", errno,
                  strerror(errno));
        return ExitCode_Init_SetDefaultTarget;
    }

    ExitCode localExitCode = ReadWhoAmI();
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    localExitCode = ResetAndSetSampleRange();
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    return ExitCode_Success;
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
    CloseFdAndPrintError(i2cFd, "i2c");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("I2C accelerometer application starting.\n");
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
