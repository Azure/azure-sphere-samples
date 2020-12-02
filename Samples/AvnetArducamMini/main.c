/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates captureing an image from a 
// attached camera and sending it to an Azure Storage account.
//
// The application will start and wait for buttonA to be pressed.  Once pressed, the 
// application will capture and image and send it to your Azure Storage Account.  
// The Red LED will light as the image is being sent to the Storage Account.

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/eventloop.h>

// Includes for the aurdcam port
#include <applibs/networking.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <applibs/storage.h>
#include "arducam_driver/ArduCAM.h"
#include "delay.h"
#include "arducam.h"
#include "exit_codes.h"

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

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

// File descriptors - initialized to invalid value
static EventLoop *eventLoop = NULL;
static int ledBlinkRateButtonGpioFd = -1;
static EventLoopTimer *buttonPollTimer = NULL;
int senbLedGpioFd = -1;

// File descriptors for the camera module
// Note these global variables get initialized/accessed from the arducam driver code
int arduCamCsFd = -1;
int arduCamSpiFd = -1;
int arduCamI2cFd = -1;

// This is the max image size the application will send to Azure Storage
// This limit will keep the applicaiton from trying to allocate more memory than is avaliable.
#define MAX_IMAGE_SIZE_SUPPORTED 150000

// Button state variables
static GPIO_Value_Type buttonState = GPIO_Value_High;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void ButtonTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Handle button timer event: if the button is pressed, change the LED blink rate.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    uint32_t image_size = 0;

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(ledBlinkRateButtonGpioFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonState;
        return;
    }

    // If the button has just been pressed, change the LED blink interval
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {

            bool isNetworkingReady = false;
            while ((Networking_IsNetworkingReady(&isNetworkingReady) < 0) || !isNetworkingReady) {
                Log_Debug("\nNot doing upload because network is not up, try again\r\n");
            }

            // Capture an image check the file size
            image_size = CaptureImage();
            Log_Debug("Captured %d bytes of image data\n", image_size);

            if (image_size < MAX_IMAGE_SIZE_SUPPORTED) {

                // Call the routine to send the file to our storage account
                UploadFileToAzureBlob(image_size);        

            } else {
                Log_Debug(
                    "ERROR: Did not transmit image, image size of %d > %d max supported image size\n",
                    image_size, MAX_IMAGE_SIZE_SUPPORTED);

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

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    ledBlinkRateButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (ledBlinkRateButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }
    struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // Open SAMPLE_LED GPIO, set as output with value GPIO_Value_High (off), and set up a timer to
    // blink it
    Log_Debug("Opening SAMPLE_LED as output.\n");
    senbLedGpioFd = GPIO_OpenAsOutput(SAMPLE_LED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (senbLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_LED GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Led;
    }

    ExitCode arduCamInitStatus = arduCamInit(ARDUCAM_CS, ARDUCAM_SPI, ARDUCAM_I2C);
    if (arduCamInitStatus != ExitCode_Success) {
        return arduCamInitStatus;
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
    // Leave the LED off
    if (senbLedGpioFd >= 0) {
        GPIO_SetValue(senbLedGpioFd, GPIO_Value_High);
    }

    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(senbLedGpioFd, "senbLedGpioFd");
    CloseFdAndPrintError(ledBlinkRateButtonGpioFd, "LedBlinkRateButtonGpio");
    CloseFdAndPrintError(arduCamCsFd, "arduCamCsFd");
    CloseFdAndPrintError(arduCamSpiFd, "arduCamSpiFd");
    CloseFdAndPrintError(arduCamI2cFd, "arduCamI2cFd");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("GPIO application starting.\n");
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