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
#include <applibs/gpio.h>
#include <applibs/eventloop.h>

#include "loadcell2.h"

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_DIRECTORY argument in the call to
// azsphere_target_hardware_definition to "HardwareDefinitions/avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
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

    ExitCode_Main_EventLoopFail = 21,
    ExitCode_Init_DataReady = 22,
    ExitCode_Init_Sample_ButtonA = 23,
    ExitCode_ButtonTimer_Consume = 24,
    ExitCode_ButtonTimer_GetButtonState = 25,
    ExitCode_Init_ButtonPollTimer = 26
} ExitCode;

// Support functions.
static void TerminationHandler(int signalNumber);
static void loadCellTimerEventHandler(EventLoopTimer *timer);
static void ButtonTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
int i2cFd = -1;
int buttonAFd = -1;
int dataReadyGPIOFd = -1;

// Button state variables
static GPIO_Value_Type buttonState = GPIO_Value_High;

loadcell2_t loadCell2;
static loadcell2_data_t cell_data;
static float weight_val;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *loadCellTimer = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// DocID026899 Rev 10, S6.1.1, I2C operation
// SDO is tied to ground so the least significant bit of the address is zero.
const uint8_t loadCellClickAddress = LOADCELL2_SLAVE_ADDRESS;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

void delay(int delaySeconds)
{
    struct timespec ts;
    ts.tv_sec = delaySeconds;
    ts.tv_nsec = 0 * 10000;
    nanosleep(&ts, NULL);
}

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
static void loadCellTimerEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AccelTimer_Consume;
        return;
    }

}

/// <summary>
///     Handle button timer event: if the button is pressed, change the LED blink rate.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    static int entryCount = 0;
    int i;   

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(buttonAFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonState;
        return;
    }

    // If the button has just been pressed, change the LED blink interval
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {
        
            // If this is the first time in, then run through the calibration steps!
            if (entryCount == 0) {
                
                entryCount++;

                loadcell2_calibrate_afe(&loadCell2);

                Log_Debug("Remove all objects from the scale\n");
                for (i = 10; i > 0; i--) {
                    Log_Debug("Tare will commense in %d seconds\n", i);
                    delay(1);
                }
                Log_Debug("\ntare the scale\n");
                loadcell2_tare(&loadCell2, &cell_data);

                Log_Debug("\nCalibrating the Scale\n");
                Log_Debug("Place 100g weight on the scale\n");
                for (i = 10; i > 0; i--) {
                    Log_Debug("Calibration will commense in %d seconds\n", i);
                    delay(1);
                }
                if (loadcell2_calibration(&loadCell2, LOADCELL2_WEIGHT_100G, &cell_data) ==
                    LOADCELL2_GET_RESULT_OK) {
                    
                    Log_Debug("\n***** Calibration complete! *****\n");
                    Log_Debug("Remove calibration weight from the scale\n");
                    Log_Debug("To use scale, place item on scale and press button A\n");
                
                } else {
                    
                    Log_Debug("Calibration error!\n");
                }

            } else {  // Else take a measurement
            
                weight_val = loadcell2_get_weight(&loadCell2, &cell_data);
                Log_Debug("Weight: %.0f g\n", weight_val);

            }


        }
        buttonState = newButtonState;
    }
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

    // Check the scale/loadCell every one second
    static const struct timespec loadCellReadPeriod = {.tv_sec = 1, .tv_nsec = 0};
    loadCellTimer = CreateEventLoopPeriodicTimer(eventLoop, &loadCellTimerEventHandler,
                                              &loadCellReadPeriod);
    if (loadCellTimer == NULL) {
        return ExitCode_Init_AccelTimer;
    }

    i2cFd = I2CMaster_Open(LOAD_CELL_2_CLICK_I2C);
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
    result = I2CMaster_SetDefaultTargetAddress(i2cFd, loadCellClickAddress);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetDefaultTargetAddress: errno=%d (%s)\n", errno,
                  strerror(errno));
        return ExitCode_Init_SetDefaultTarget;
    }

    // Setup the GPIO signal for the load cell 2 click data ready signal
    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Open Data Ready GPIO %d as input.\n", LOAD_CELL_2_CLICK_DATA_READY);
    dataReadyGPIOFd = GPIO_OpenAsInput(LOAD_CELL_2_CLICK_DATA_READY);
    if (dataReadyGPIOFd == -1) {
        Log_Debug("ERROR: Could not open LOAD_CELL_2_CLICK_DATA_READY: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_DataReady;
    }

    // Setup the GPIO signal for the button a signal
    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Open SAMPLE_BUTTON_1 as input.\n");
    buttonAFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonAFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Sample_ButtonA;
    }

    struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // We've created the hardware file descriptors, use them to initialize the 
    // loadCell2 object
    loadCell2.i2c = i2cFd;
    loadCell2.rdy = dataReadyGPIOFd;
    loadCell2.slave_address = LOADCELL2_SLAVE_ADDRESS;

    // Initialize the Load Cell 2 click board
    loadcell2_reset(&loadCell2);
    
    if (loadcell2_power_on(&loadCell2) == LOADCELL2_ERROR) {
    
        Log_Debug("loadcell2_power_on() failed!\n");
    }
   
    loadcell2_default_cfg(&loadCell2);

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
    DisposeEventLoopTimer(loadCellTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(i2cFd, "i2c");
    CloseFdAndPrintError(dataReadyGPIOFd, "dataReadyGPIOFd");
    CloseFdAndPrintError(buttonAFd, "buttonAFd");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("I2C Load Cell application starting.\n");
    exitCode = InitPeripheralsAndHandlers();
    Log_Debug("\nPress button A to start scale calibration\n\n");

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
