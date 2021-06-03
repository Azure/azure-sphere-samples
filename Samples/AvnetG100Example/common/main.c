/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application demonstrates how to use Azure Sphere devices with Azure IoT
// services, using the Azure IoT C SDK.
//
// It implements a simulated thermometer device, with the following features:
// - Telemetry upload (simulated temperature, device moved events) using Azure IoT Hub events.
// - Reporting device state (serial number) using device twin/read-only properties.
// - Mutable device state (telemetry upload enabled) using device twin/writeable properties.
// - Alert messages invoked from the cloud using device methods.
//
// It can be configured using the top-level CMakeLists.txt to connect either directly to an
// Azure IoT Hub, to an Azure IoT Edge device, or to use the Azure Device Provisioning service to
// connect to either an Azure IoT Hub, or an Azure IoT Central application. All connection types
// make use of the device certificate issued by the Azure Sphere security service to authenticate,
// and supply an Azure IoT PnP model ID on connection.
//
// It uses the following Azure Sphere libraries:
// - eventloop (system invokes handlers for timer events)
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - networking (network interface connection status)
//
// You will need to provide information in the application manifest to use this application. Please
// see README.md and the other linked documentation for full details.

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <applibs/log.h>

#include "eventloop_timer_utilities.h"
#include "user_interface.h"
#include "exitcodes.h"
#include "cloud.h"
#include "options.h"
#include "connection.h"
#if defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    
#include "linkedList.h"
#endif 
// Avnet specific includes
#include "build_options.h"
#include "../avnet/device_twin.h"
#include "../avnet/direct_methods.h"
#include "../avnet/iotConnect.h"

// If we have real time applications, include the support implementation
#ifdef M4_INTERCORE_COMMS
#include "../avnet/m4_support.h"
#endif 

// Global variable used to report any fatal errors in the application.  Application code will set
// this variable to a non ExitCode_Success value and the main() loop will exit.
volatile sig_atomic_t exitCode = ExitCode_Success;

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
void ClosePeripheralsAndHandlers(void);

// Interface callbacks
static void ExitCodeCallbackHandler(ExitCode ec);

#ifdef IOT_HUB_APPLICATION
// Cloud
static void ConnectionChangedCallbackHandler(bool connected);
#endif 

// Timers / polling
EventLoop *eventLoop = NULL;
EventLoopTimer *telemetryTimer = NULL;  // Drives how often telemetry data is transmitted
EventLoopTimer *sensorPollTimer = NULL; // Drives how often local sensors are read

#ifdef IOT_HUB_APPLICATION
static bool isConnected = false;
#endif // IOT_HUB_APPLICATION
// Business logic

// Avnet additions
static void ReadWifiConfig(bool);
static void ReadSensorTimerEventHandler(EventLoopTimer *timer);

// Variable used to update sensorPollTimer
int readSensorPeriod = SENSOR_READ_PERIOD_SECONDS;

typedef struct
{
	uint8_t SSID[WIFICONFIG_SSID_MAX_LENGTH];
	uint32_t frequency_MHz;
	int8_t rssi;
} network_var;

// Global variable to hold wifi network configuration data
network_var network_data;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Avnet Default Application starting.\n");

    // Read the current wifi configuration, output debug
    ReadWifiConfig(true);

#ifdef IOT_HUB_APPLICATION
    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    exitCode = Options_ParseArgs(argc, argv);

    if (exitCode != ExitCode_Success) {
        return exitCode;
    }
#endif 
    exitCode = InitPeripheralsAndHandlers();

    // Main loop
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

static void ExitCodeCallbackHandler(ExitCode ec)
{
    exitCode = ec;
}

#ifdef IOT_HUB_APPLICATION
static void DisplayAlertCallbackHandler(const char *alertMessage)
{
    Log_Debug("ALERT: %s\n", alertMessage);
}

static void ConnectionChangedCallbackHandler(bool connected)
{
    // Update the global connected status variable
    isConnected = connected;

    if (isConnected) {

        // Send up device and application details as read only device twin updates.  These constants
        // are defined in the build_options.h file
        Cloud_Result result =  updateDeviceTwin(false, ARGS_PER_TWIN_ITEM*3, 
                                                                TYPE_STRING, "versionString", VERSION_STRING, 
                                                                TYPE_STRING, "manufacturer", DEVICE_MFG,
                                                                TYPE_STRING, "model", DEVICE_MODEL);
        if (result != Cloud_Result_OK) {
            Log_Debug("WARNING: Could not send device details to cloud: %s\n",
                      CloudResultToString(result));
        }

#if defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    
        // Check to see if we have any unsent telemetry messages.  If so, then resend them.
        if(head != NULL){

            // Setup a node pointer to the first item in the list
            telemetryNode_t* currentNode = head;

            do{
                
                Log_Debug("Attempting to resend telemetry after reconnect!\n");

                // Attempt to send the message again using the same linked list node
                AzureIoT_Result aziotResult = AzureIoT_SendTelemetry(currentNode->telemetryJson, currentNode);
                Cloud_Result result = AzureIoTToCloudResult(aziotResult);

                // If the send fails, output a message
                if (result != Cloud_Result_OK) {
                    Log_Debug("WARNING: Could not send telemetry to cloud: %s.\n", CloudResultToString(result));
                }

                // Move the pointer to the next item in the list
                currentNode = currentNode->next;

            }while (currentNode != NULL);

        }
#endif // defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    

#ifdef USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS
        // Since the connection state just changed, update the status LEDs
        updateConnectionStatusLed();
#endif

#ifdef USE_IOT_CONNECT
        // Call the routine that will send the hello message to IoTConnect
        IoTConnectConnectedToIoTHub();
#endif

        // Send the current device twin properties.
        sendInitialDeviceTwinReportedProperties();

        // Read the current wifi configuration
        ReadWifiConfig(true);        
    }
}
#endif // IOT_HUB_APPLICATION

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

#ifdef IOT_HUB_APPLICATION
    // Iterate across all the device twin items and open any File Descriptors
    deviceTwinOpenFDs();

     // Initialize the direct method handler.  Each direct method can implement its own
     // specific init routine.  
    ExitCode result = InitDirectMethods();
    if ( result != ExitCode_Success){
        return result;
    }

#endif // IOT_HUB_APPLICATION

#ifdef M4_INTERCORE_COMMS
    // Iterate across all Real time application init routines.  Each real time application
    // can implement its own specific routine
    ExitCode m4ReturnStatus = InitM4Interfaces();
    if (m4ReturnStatus != ExitCode_Success){
        return m4ReturnStatus;
    }
#endif

    // Initialize the button user interface
    ExitCode interfaceExitCode =
        UserInterface_Initialise(eventLoop, NULL, ExitCodeCallbackHandler);

    if (interfaceExitCode != ExitCode_Success) {
        return interfaceExitCode;
    }

    // Set up a timer to poll the sensors.  SENSOR_READ_PERIOD_SECONDS is defined in build_options.h
    static const struct timespec readSensorPeriod = {.tv_sec = SENSOR_READ_PERIOD_SECONDS,
                                                     .tv_nsec = SENSOR_READ_PERIOD_NANO_SECONDS};
    sensorPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ReadSensorTimerEventHandler, &readSensorPeriod);
    if (sensorPollTimer == NULL) {
        return ExitCode_Init_sensorPollTimer;
    }

#ifdef IOT_HUB_APPLICATION    
    void *connectionContext = Options_GetConnectionContext();

    return Cloud_Initialize(eventLoop, connectionContext, ExitCodeCallbackHandler,
                            DisplayAlertCallbackHandler, ConnectionChangedCallbackHandler);
#else 
    return ExitCode_Success;
#endif 
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(telemetryTimer);
    DisposeEventLoopTimer(sensorPollTimer);
    Cloud_Cleanup();
    UserInterface_Cleanup();
    Connection_Cleanup();
    EventLoop_Close(eventLoop);

#ifdef M4_INTERCORE_COMMS    
    CleanupM4Resources();
#endif 

    Log_Debug("Closing file descriptors\n");
}

// Read the current wifi configuration, output it to debug and send it up as device twin data
static void ReadWifiConfig(bool outputDebug){
   	
    char bssid[20];
#ifdef IOT_HUB_APPLICATION        
    static bool ssidChanged = false;
#endif     

	WifiConfig_ConnectedNetwork network;
	int result = WifiConfig_GetCurrentNetwork(&network);

	if (result < 0) 
	{
	    // Log_Debug("INFO: Not currently connected to a WiFi network.\n");
		strncpy(network_data.SSID, "Not Connected", 20);
		network_data.frequency_MHz = 0;
		network_data.rssi = 0;
	}
	else 
	{

        network_data.frequency_MHz = network.frequencyMHz;
        network_data.rssi = network.signalRssi;
		snprintf(bssid, JSON_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x",
			network.bssid[0], network.bssid[1], network.bssid[2], 
			network.bssid[3], network.bssid[4], network.bssid[5]);

        // Check to see if the SSID changed, if so update the SSID and send updated device twins properties
		if (strncmp(network_data.SSID, (char*)&network.ssid, network.ssidLength)!=0 ) {

#ifdef IOT_HUB_APPLICATION    
            // Set the flag to send ssid changes to the IoTHub
            ssidChanged = true;
#endif             

			// Clear the ssid array
			memset(network_data.SSID, 0, WIFICONFIG_SSID_MAX_LENGTH);
			strncpy(network_data.SSID, network.ssid, network.ssidLength);
        }

#ifdef IOT_HUB_APPLICATION

        if((iothubClientHandle != NULL) && (ssidChanged)){

            updateDeviceTwin(false, ARGS_PER_TWIN_ITEM*3, 
                                    TYPE_STRING, "ssid", &network_data.SSID,
                                    TYPE_INT, "freq", network_data.frequency_MHz,
                                    TYPE_STRING, "bssid", &bssid);               
            
            // Reset the flag 
            ssidChanged = false;
        }
#endif
        if(outputDebug){

		    Log_Debug("SSID: %s\n", network_data.SSID);
		    Log_Debug("Frequency: %dMHz\n", network_data.frequency_MHz);
		    Log_Debug("bssid: %s\n", bssid);
            Log_Debug("rssi: %d\n", network_data.rssi);
        }
    }
}

/// <summary>
///     Senspr timer event:  Read the sensors
/// </summary>
static void ReadSensorTimerEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ReadSensorTimer_Consume;
        return;
    }

    // Add code here to read any sensors attached to the device

#ifdef M4_INTERCORE_COMMS
    // Send each real time core a message requesting raw data
    // Each real time application must implement this functionality
    RequestRawData();
#endif     

    // Read the current wifi configuration
    ReadWifiConfig(false);

    // Call the routine to read/report the application's high water memory usage
    // This routine will send a device twin update if the high water mark increased
    checkMemoryUsageHighWaterMark();


}

