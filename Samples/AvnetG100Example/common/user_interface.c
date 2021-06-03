/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/log.h>

#include "user_interface.h"
#include "build_options.h"

#include "../avnet/device_twin.h"

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

void CloseFdAndPrintError(int fd, const char *fdName);

static EventLoopTimer *buttonPollTimer = NULL;

static ExitCode_CallbackType failureCallbackFunction = NULL;
static UserInterface_ButtonPressedCallbackType buttonPressedCallbackFunction = NULL;

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
