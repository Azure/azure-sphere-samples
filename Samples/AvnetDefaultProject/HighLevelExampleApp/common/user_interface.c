/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>

#include <applibs/gpio.h>
#include <applibs/log.h>

#include "user_interface.h"
#include "build_options.h"
#include "../avnet/oled.h"

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_HARDWARE variable to
// "avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
#include <hw/sample_appliance.h>

#include "eventloop_timer_utilities.h"

#ifndef GUARDIAN_100
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
#endif // !GUARDIAN_100

#ifdef OLED_SD1306
static void UpdateOledEventHandler(EventLoopTimer *timer);
#endif

void CloseFdAndPrintError(int fd, const char *fdName);

#ifndef GUARDIAN_100
// File descriptors - initialized to invalid value
static int buttonAGpioFd = -1;
static int buttonBGpioFd = -1;
#endif 

static EventLoopTimer *buttonPollTimer = NULL;
#ifdef OLED_SD1306
static EventLoopTimer *oledUpdateTimer = NULL;
#endif 

static ExitCode_CallbackType failureCallbackFunction = NULL;
static UserInterface_ButtonPressedCallbackType buttonPressedCallbackFunction = NULL;

#ifndef GUARDIAN_100
// State variables
static GPIO_Value_Type buttonAState = GPIO_Value_High;
static GPIO_Value_Type buttonBState = GPIO_Value_High;
#endif // !GUARDIAN_100

#if (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))
#define RGB_NUM_LEDS 3
//  RGB LEDs
static int gpioConnectionStateLedFds[RGB_NUM_LEDS] = {-1, -1, -1};
static GPIO_Id gpioConnectionStateLeds[RGB_NUM_LEDS] = {LED_1, LED_2, LED_3};

// Using the bits set in networkStatus, turn on/off the status LEDs
void setConnectionStatusLed(RGB_Status networkStatus)
{
    GPIO_SetValue(gpioConnectionStateLedFds[RGB_LED1_INDEX],
                  (networkStatus & (1 << RGB_LED1_INDEX)) ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(gpioConnectionStateLedFds[RGB_LED2_INDEX],
                  (networkStatus & (1 << RGB_LED2_INDEX)) ? GPIO_Value_Low : GPIO_Value_High);
    GPIO_SetValue(gpioConnectionStateLedFds[RGB_LED3_INDEX],
                  (networkStatus & (1 << RGB_LED3_INDEX)) ? GPIO_Value_Low : GPIO_Value_High);
}
#endif // (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))

#ifndef GUARDIAN_100
/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        failureCallbackFunction(ExitCode_IsButtonPressed_GetValue);
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Button timer event:  Check the status of the button
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        failureCallbackFunction(ExitCode_ButtonTimer_Consume);
        return;
    }

    if (IsButtonPressed(buttonAGpioFd, &buttonAState) && NULL != buttonPressedCallbackFunction) {

        // This callback is set from main.c with a call to UserInterface_Initialise()
        buttonPressedCallbackFunction(UserInterface_Button_A);
    }

    if (IsButtonPressed(buttonBGpioFd, &buttonBState) && NULL != buttonPressedCallbackFunction) {
        
        // This callback is set from main.c with a call to UserInterface_Initialise()
        buttonPressedCallbackFunction(UserInterface_Button_B);
    }
}
#endif // !GUARDIAN_100    

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

ExitCode UserInterface_Initialise(EventLoop *el,
                                  UserInterface_ButtonPressedCallbackType buttonPressedCallback,
                                  ExitCode_CallbackType failureCallback)
{
    failureCallbackFunction = failureCallback;
    buttonPressedCallbackFunction = buttonPressedCallback;

#ifndef GUARDIAN_100
    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    buttonAGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonAGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    buttonBGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (buttonBGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_Button;
    }

    // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(el, &ButtonPollTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }
#endif // !GUARDIAN_100

#ifdef OLED_SD1306

    // Initialize the i2c buss to drive the OLED
    lp_imu_initialize();

    // Set up a timer to drive quick oled updates.
    static const struct timespec oledUpdatePeriod = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};
    oledUpdateTimer = CreateEventLoopPeriodicTimer(el, &UpdateOledEventHandler,
                                                   &oledUpdatePeriod);
    if (oledUpdateTimer == NULL) {
        return ExitCode_Init_OledUpdateTimer;
    }
#endif 

#if (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))    // Initailize the user LED FDs,
    for (int i = 0; i < RGB_NUM_LEDS; i++) {
        gpioConnectionStateLedFds[i] = GPIO_OpenAsOutput(gpioConnectionStateLeds[i],
                                                         GPIO_OutputMode_PushPull, GPIO_Value_High);
        if (gpioConnectionStateLedFds[i] < 0) {
            Log_Debug("ERROR: Could not open LED GPIO: %s (%d).\n", strerror(errno), errno);
            return ExitCode_Init_StatusLeds;
        }
    }
#endif // (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))

    return ExitCode_Success;
}

void UserInterface_Cleanup(void)
{
    DisposeEventLoopTimer(buttonPollTimer);

#ifndef GUARDIAN_100
    CloseFdAndPrintError(buttonAGpioFd, "ButtonA");
    CloseFdAndPrintError(buttonBGpioFd, "ButtonB");
#endif

#ifdef OLED_SD1306
    DisposeEventLoopTimer(oledUpdateTimer);
#endif     

#if (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))    // Turn the WiFi connection status LEDs off
    setConnectionStatusLed(RGB_No_Connections);

    // Close the status LED file descriptors
    for (int i = 0; i < RGB_NUM_LEDS; i++) {
        CloseFdAndPrintError(gpioConnectionStateLedFds[i], "ConnectionStatusLED");
    }
#endif // (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))

}

#if (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))
// Determine the network status and call the routine to set the status LEDs
void updateConnectionStatusLed(void)
{
    RGB_Status networkStatus;
    bool bIsNetworkReady = false;

    if (Networking_IsNetworkingReady(&bIsNetworkReady) < 0) {
        networkStatus = RGB_No_Connections; // network error
    } else {
        networkStatus = !bIsNetworkReady ? RGB_No_Network // no Network, No WiFi
                : (iotHubClientAuthenticationState == IoTHubClientAuthenticationState_Authenticated)
                ? RGB_IoT_Hub_Connected   // IoT hub connected
                : RGB_Network_Connected; // only Network connected
    }

    // Set the LEDs based on the current status
    setConnectionStatusLed(networkStatus);
}
#endif // (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))

#ifdef OLED_SD1306
/// <summary>
///     OLED timer handler: refresh the OLED screen/data
/// </summary>
static void UpdateOledEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

	// Update/refresh the OLED data
	update_oled();
}

#endif // OLED_SD1306

/// <summary>
///     Read and manage the memory high water mark
///     This should never exceed 256KB for the MT3620
/// </summary>
void checkMemoryUsageHighWaterMark(void)
{

    static size_t memoryHighWaterMark = 0;
    size_t currentMax = 0;

    // Read out process and display the memory usage high water mark
    //
    // MSFT documentation
    // https://docs.microsoft.com/en-us/azure-sphere/app-development/application-memory-usage?pivots=vs-code#determine-run-time-application-memory-usage
    //
    // Applications_GetPeakUserModeMemoryUsageInKB:
    // Get the peak user mode memory usage in kibibytes.This is the maximum amount of user 
    // memory used in the current session.When testing memory usage of your application,
    // you should ensure this value never exceeds 256 KiB.This value resets whenever your app
    // restarts or is redeployed.
    // Use this function to get an approximate look into how close your application is
    // getting to the 256 KiB recommended limit.

    currentMax = Applications_GetPeakUserModeMemoryUsageInKB();

    // Check to see if we have a new high water mark.  If so, send up a device twin update
    if(currentMax > memoryHighWaterMark){

        memoryHighWaterMark = currentMax;
        Log_Debug("New Memory High Water Mark: %d KiB\n", memoryHighWaterMark);

#ifdef IOT_HUB_APPLICATION    
        
        // Send the reported property to the IoTHub    
        updateDeviceTwin(true, ARGS_PER_TWIN_ITEM*1, TYPE_INT, "MemoryHighWaterKB", (int)memoryHighWaterMark);

#endif         
    }
}
