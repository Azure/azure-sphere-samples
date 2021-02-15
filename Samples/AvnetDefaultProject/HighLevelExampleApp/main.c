/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application demonstrates how to interface Azure Sphere devices with Azure IoT
// services. Using the Azure IoT SDK C APIs, it shows how to:
// 1. Use Device Provisioning Service (DPS) to connect to Azure IoT Hub/Central with
// certificate-based authentication
// 2. Use X.509 Certificate Authority (CA) certificates to authenticate devices connecting directly
// to Azure IoT Hub
// 3. Use X.509 Certificate Authority (CA) certificates to authenticate devices connecting to an
// IoT Edge device.
// 4. Use Azure IoT Hub messaging to upload simulated temperature measurements and to signal button
// press events
// 5. Use Device Twin to receive desired LED state from the Azure IoT Hub
// 6. Use Direct Methods to receive a "Trigger Alarm" command from Azure IoT Hub/Central
//
// It uses the following Azure Sphere libraries:
// - eventloop (system invokes handlers for timer events)
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - networking (network interface connection status)
// - storage (device storage interaction)
//
// You will need to provide information in the 'CmdArgs' section of the application manifest to
// use this application. Please see README.md for full details.

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "build_options.h"
#ifdef USE_PNP
#include <prov_security_factory.h>
#include <prov_transport_mqtt_client.h>
#include <applibs/application.h>
#endif 

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/eventloop.h>
#include <applibs/gpio.h>
#include <applibs/wificonfig.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>
#include <applibs/powermanagement.h>

#ifdef M4_INTERCORE_COMMS
//// ADC connection
#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/application.h>
#endif 

// Add support for the on-board sensors
#include "i2c.h"

// Add support for the OLED display
#include "oled.h"

#include "exit_codes.h"

// Add support for managing device twins from a structure
#include "deviceTwin.h"
#include "direct_methods.h"
#include "iotConnect.h"

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
#include "parson.h" // Used to parse Device Twin messages.

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>
#include <iothub_security_factory.h>
#include <shared_util_options.h>

#ifdef USE_PNP
#define dpsUrl "global.azure-devices-provisioning.net"
static PROV_DEVICE_RESULT dpsRegisterStatus = PROV_DEVICE_REG_HUB_NOT_SPECIFIED;
static char* iotHubUri = NULL;
#endif 
volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
/// Connection types to use when connecting to the Azure IoT Hub.
/// </summary>
typedef enum {
    ConnectionType_NotDefined = 0,
    ConnectionType_DPS,
    ConnectionType_Direct,
#ifdef USE_PNP    
    ConnectionType_PnP,
#endif     
    ConnectionType_IoTEdge
    
} ConnectionType;

/// <summary>
/// Authentication state of the client with respect to the Azure IoT Hub.
/// </summary>
typedef enum {
    /// <summary>Client is not authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_NotAuthenticated = 0,
    /// <summary>Client has initiated authentication to the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_AuthenticationInitiated = 1,
    /// <summary>Client is authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_Authenticated = 2
} IoTHubClientAuthenticationState;

// Constants
#define TELEMETRY_BUFFER_SIZE 100
#define MAX_ROOT_CA_CERT_CONTENT_SIZE (3 * 1024)

#ifdef IOT_HUB_APPLICATION

// Azure IoT definitions.
static char *scopeId = NULL;  // ScopeId for DPS.
static char *hostName = NULL; // Azure IoT Hub or IoT Edge Hostname.
static ConnectionType connectionType = ConnectionType_NotDefined; // Type of connection to use.
static char *iotEdgeRootCAPath = NULL; // Path (including filename) of the IotEdge cert.
static char iotEdgeRootCACertContent[MAX_ROOT_CA_CERT_CONTENT_SIZE +
                                     1]; // Add 1 to account for null terminator.
static IoTHubClientAuthenticationState iotHubClientAuthenticationState =
    IoTHubClientAuthenticationState_NotAuthenticated; // Authentication state with respect to the
                                                      // IoT Hub.

IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int deviceIdForDaaCertUsage = 1; // A constant used to direct the IoT SDK to use
                                              // the DAA cert under the hood.
static const char networkInterface[] = "wlan0";

// Function declarations
void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context);
void ReportedStateCallback(int result, void *context);
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);
void SendTelemetry(const char *jsonMessage, bool);
static void SetUpAzureIoTHubClient(void);
#endif // IOT_HUB_APPLICATION
static void ReadWifiConfig(bool);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static bool ButtonStateChanged(int fd, GPIO_Value_Type *oldState);
static void ReadSensorTimerEventHandler(EventLoopTimer *timer);
static void SendTelemetryTimerEventHandler(EventLoopTimer *timer);

#ifdef OLED_SD1306
static void UpdateOledEventHandler(EventLoopTimer *timer);
#endif

#ifdef IOT_HUB_APPLICATION
static void AzureTimerEventHandler(EventLoopTimer *timer);
static ExitCode ValidateUserConfiguration(void);
static void ParseCommandLineArguments(int argc, char *argv[]);
static bool SetUpAzureIoTHubClientWithDaa(void);
static bool SetUpAzureIoTHubClientWithDps(void);
#ifdef USE_PNP
static bool ProvisionWithDpsPnP(void);
#endif
bool IsConnectionReadyToSendTelemetry(void);
static ExitCode ReadIoTEdgeCaCertContent(void);
#endif // IOT_HUB_APPLICATION
// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);
static void TriggerReboot(void);

// File descriptors - initialized to invalid value

// Buttons
static int buttonAgpioFd = -1;
static int buttonBgpioFd = -1;

// GPIO File descriptors
int userLedRedFd = -1;
int userLedGreenFd = -1;
int userLedBlueFd = -1;

// Variables to track LED state
bool userLedRedIsOn = false;
bool userLedGreenIsOn = false;
bool userLedBlueIsOn = false;

#ifdef M4_INTERCORE_COMMS
//// ADC connection
static const char rtAppComponentId[] = "005180bc-402f-4cb3-a662-72937dbcde47";
static int sockFd = -1;
static void SendMessageToRTCore(void);
static void M4TimerEventHandler(EventLoopTimer *timer);
static void SocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
uint8_t RTCore_status;
static EventLoopTimer *M4PollTimer = NULL;
static EventRegistration *rtAppEventReg = NULL;
#endif 

// Global variable to hold wifi network configuration data
network_var network_data;

float altitude;

// Timer / polling
EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;
EventLoopTimer *telemetrytxIntervalr = NULL;
EventLoopTimer *sensorPollTimer = NULL;
EventLoopTimer *rebootDeviceTimer = NULL;

#ifdef OLED_SD1306
static EventLoopTimer *oledUpdateTimer = NULL;
#endif 

#ifdef IOT_HUB_APPLICATION
static EventLoopTimer *azureTimer = NULL;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 1;        // poll azure iot every second
static const int AzureIoTMinReconnectPeriodSeconds = 60;      // back off when reconnecting
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60; // back off limit

static int azureIoTPollPeriodSeconds = -1;

#endif // IOT_HUB_APPLICATION

// State variables
static GPIO_Value_Type buttonAState = GPIO_Value_High;
static GPIO_Value_Type buttonBState = GPIO_Value_High;

#ifdef IOT_HUB_APPLICATION


// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "DPS connection type: \" CmdArgs \": [\"--ConnectionType\", \"DPS\", \"--ScopeID\", "
    "\"<scope_id>\"]\n"
#ifdef USE_PNP     
    "PnP connection type: \" CmdArgs \": [\"--ConnectionType\", \"PnP\", \"--ScopeID\", "
    "\"<scope_id>\"]\n"
#endif     
    "Direction connection type: \" CmdArgs \": [\"--ConnectionType\", \"Direct\", "
    "\"--Hostname\", \"<azureiothub_hostname>\"]\n "
    "IoTEdge connection type: \" CmdArgs \": [\"--ConnectionType\", \"IoTEdge\", "
    "\"--Hostname\", \"<iotedgedevice_hostname>\", \"--IoTEdgeRootCAPath\", "
    "\"certs/<iotedgedevice_cert_name>\"]\n";
#endif // IOT_HUB_APPLICATION

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
    Log_Debug("Avnet Starter Kit Simple Reference Application starting.\n");

    // Read the current wifi configuration, output debug
    ReadWifiConfig(true);

#ifdef IOT_HUB_APPLICATION
    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    ParseCommandLineArguments(argc, argv);

    exitCode = ValidateUserConfiguration();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    if (connectionType == ConnectionType_IoTEdge) {
        exitCode = ReadIoTEdgeCaCertContent();
        if (exitCode != ExitCode_Success) {
            return exitCode;
        }
    }
    
#endif // IOT_HUB_APPLICATION

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

    if (exitCode == ExitCode_TriggerReboot_Success) {
        TriggerReboot();
    }

    return exitCode;
}

/// <summary>
///     Button timer event:  Check the status of the button
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
#ifdef IOT_HUB_APPLICATION    
    // Flags used to dertermine if we need to send a telemetry update or not
   	bool sendTelemetryButtonA = false;
	bool sendTelemetryButtonB = false;
#endif     

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    if (ButtonStateChanged(buttonAgpioFd, &buttonAState)) {
    
   		if (buttonAState == GPIO_Value_Low) {
    		Log_Debug("Button A pressed!\n");
#ifdef IOT_HUB_APPLICATION    	    	
            sendTelemetryButtonA = true;
#endif 
#ifdef OLED_SD1306
            // Use buttonB presses to drive OLED to display the last screen
	    	oled_state--;
        
		    if (oled_state < 0 )
		    {
    			oled_state = OLED_NUM_SCREEN;
	    	}

            Log_Debug("OledState: %d\n", oled_state);
#endif 
	    }
	    else {
		    Log_Debug("Button A released!\n");
	    }

    }

	// If the B button has just been pressed/released, send a telemetry message
	// The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (ButtonStateChanged(buttonBgpioFd, &buttonBState)) {

    	if (buttonBState == GPIO_Value_Low) {
			Log_Debug("Button B pressed!\n");
#ifdef IOT_HUB_APPLICATION    		
        	sendTelemetryButtonB = true;
#endif             

#ifdef OLED_SD1306
            // Use buttonB presses to drive OLED to display the next screen
	    	oled_state++;
        
		    if (oled_state > OLED_NUM_SCREEN)
		    {
    			oled_state = 0;
	    	}
            Log_Debug("OledState: %d\n", oled_state);
#endif 
		}
		else {
			Log_Debug("Button B released!\n");
		}
	}
	
#ifdef IOT_HUB_APPLICATION    	

    // If either button was pressed, then enter the code to send the telemetry message
 	if (sendTelemetryButtonA || sendTelemetryButtonB) {

	    char *pjsonBuffer = (char *)malloc(JSON_BUFFER_SIZE);
   		if (pjsonBuffer == NULL) {
 			Log_Debug("ERROR: not enough memory to send telemetry");
            exitCode = ExitCode_Button_Telemetry_Malloc_Failed;
   		}

	    if (sendTelemetryButtonA) {
  			// construct the telemetry message  for Button A
		    snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonInteger, "buttonA", buttonAState);
	    }

	    if (sendTelemetryButtonB) {
		    // construct the telemetry message for Button B
		    snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonInteger, "buttonB", buttonBState);
            
        }

   		Log_Debug("\n[Info] Sending telemetry %s\n", pjsonBuffer);
        SendTelemetry(pjsonBuffer, true);
	    free(pjsonBuffer);
    }
#endif // IOT_HUB_APPLICATION
}

#ifdef OLED_SD1306
/// <summary>
///     Button timer event:  Check the status of the button
/// </summary>
static void UpdateOledEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

	// Update/refresh the OLED data
	update_oled();
}

#endif 

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
    Log_Debug("ALSPT19: Ambient Light[Lux] : %.2f\r\n", light_sensor);
#endif

    // Read the current wifi configuration
    ReadWifiConfig(false);

#ifdef IOT_HUB_APPLICATION
#ifdef USE_IOT_CONNECT
    // If we have not completed the IoTConnect connect sequence, then don't send telemetry
    if(IoTCConnected){
#endif         

#ifdef USE_IOT_CONNECT        
    }
#endif // USE_IOT_CONNECT
#endif // IOT_HUB_APPLICATION    
}

#ifdef IOT_HUB_APPLICATION
/// <summary>
///     Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AzureTimer_Consume;
        return;
    }

    // Check whether the device is connected to the internet.
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(networkInterface, &status) == 0) {
        if ((status & Networking_InterfaceConnectionStatus_ConnectedToInternet) &&
            (iotHubClientAuthenticationState == IoTHubClientAuthenticationState_NotAuthenticated)) {
            SetUpAzureIoTHubClient();

#ifdef USE_IOT_CONNECT
            // Kick off the IoTConnect specific logic since we're connected!
            IoTConnectConnectedToIoTHub();
#endif

        }
    } else {
        if (errno != EAGAIN) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                      strerror(errno));
            exitCode = ExitCode_InterfaceConnectionStatus_Failed;
            return;
        }
    }

    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Senspr timer event:  Read the sensors
/// </summary>
static void SendTelemetryTimerEventHandler(EventLoopTimer *timer)
{

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }

    // Allocate memory for a telemetry message to Azure
    char *pjsonBuffer = (char *)malloc(JSON_BUFFER_SIZE);
    if (pjsonBuffer == NULL) {
        Log_Debug("ERROR: not enough memory to send telemetry");
    }

    snprintf(pjsonBuffer, JSON_BUFFER_SIZE,
         "{\"sampleKeyString\":\"%s\", \"sampleKeyInt\":%d, \"sampleKeyFloat\":%.3lf}", "Rush", 2112, 12.345);
    Log_Debug("\n[Info] Sending telemetry: %s\n", pjsonBuffer);
    SendTelemetry(pjsonBuffer, true);
    free(pjsonBuffer);

}

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
static void ParseCommandLineArguments(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "ConnectionType", .has_arg = required_argument, .flag = NULL, .val = 'c'},
        {.name = "ScopeID", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'},
        {.name = "IoTEdgeRootCAPath", .has_arg = required_argument, .flag = NULL, .val = 'i'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "c:s:h:i:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 'c':
            Log_Debug("ConnectionType: %s\n", optarg);
            if (strcmp(optarg, "DPS") == 0) {
                connectionType = ConnectionType_DPS;
            } else if (strcmp(optarg, "Direct") == 0) {
                connectionType = ConnectionType_Direct;
            } else if (strcmp(optarg, "IoTEdge") == 0) {
                connectionType = ConnectionType_IoTEdge;
            }
#ifdef USE_PNP            
            else if (strcmp(optarg, "PnP") == 0){
                connectionType = ConnectionType_PnP;
            }
#endif             
            break;
        case 's':
            Log_Debug("ScopeID: %s\n", optarg);
            scopeId = optarg;
            break;
        case 'h':
            Log_Debug("Hostname: %s\n", optarg);
            hostName = optarg;
            break;
        case 'i':
            Log_Debug("IoTEdgeRootCAPath: %s\n", optarg);
            iotEdgeRootCAPath = optarg;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }
}

/// <summary>
///     Validates if the values of the Connection type, Scope ID, IoT Hub or IoT Edge Hostname
///  were set.
/// </summary>
/// <returns>ExitCode_Success if the parameters were provided; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (connectionType < ConnectionType_DPS || connectionType > ConnectionType_IoTEdge) {
        validationExitCode = ExitCode_Validate_ConnectionType;
    }
    if (connectionType == ConnectionType_DPS) {
        if (scopeId == NULL) {
            validationExitCode = ExitCode_Validate_ScopeId;
        } else {
            Log_Debug("Using DPS Connection: Azure IoT DPS Scope ID %s\n", scopeId);
        }
    }
    if (connectionType == ConnectionType_Direct) {
        if (hostName == NULL) {
            validationExitCode = ExitCode_Validate_Hostname;
        }

        if (validationExitCode == ExitCode_Success) {
            Log_Debug("Using Direct Connection: Azure IoT Hub Hostname %s\n", hostName);
        }
    }
    #ifdef USE_PNP
    if (connectionType == ConnectionType_PnP) {
        if (scopeId == NULL) {
            validationExitCode = ExitCode_Validate_ScopeId;
        } else {
            Log_Debug("Using DPS Connection: Azure IoT DPS Scope ID %s\n", scopeId);
        }
    }
#endif 

    if (connectionType == ConnectionType_IoTEdge) {
        if (hostName == NULL) {
            validationExitCode = ExitCode_Validate_Hostname;
        }

        if (iotEdgeRootCAPath == NULL) {
            validationExitCode = ExitCode_Validate_IoTEdgeCAPath;
        }

        if (validationExitCode == ExitCode_Success) {
            Log_Debug("Using IoTEdge Connection: IoT Edge device Hostname %s, IoTEdge CA path %s\n",
                      hostName, iotEdgeRootCAPath);
        }
    }

    if (validationExitCode != ExitCode_Success) {
        Log_Debug("Command line arguments for application shoud be set as below\n%s",
                  cmdLineArgsUsageText);
    }

    return validationExitCode;
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

    // Open SAMPLE_BUTTON_1 GPIO as input (ButtonA)
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    buttonAgpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonAgpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonA;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input (ButtonB)
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    buttonBgpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (buttonBgpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonB;
    }
  
        // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }


#ifdef OLED_SD1306
    // Set up a timer to drive quick oled updates.
    static const struct timespec oledUpdatePeriod = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};
    oledUpdateTimer = CreateEventLoopPeriodicTimer(eventLoop, &UpdateOledEventHandler,
                                                   &oledUpdatePeriod);
    if (oledUpdateTimer == NULL) {
        return ExitCode_Init_OledUpdateTimer;
    }
#endif 
    
    // Iterate across all the device twin items and open any File Descriptors
    deviceTwinOpenFDs();

    // Initialize the direct method handler
    if (InitDirectMethods() != ExitCode_Success){
        return ExitCode_Init_DirectMethods;
    }

    // Set up a timer to poll the sensors.  SENSOR_READ_PERIOD_SECONDS is defined in build_options.h
    static const struct timespec readSensorPeriod = {.tv_sec = SENSOR_READ_PERIOD_SECONDS,
                                                     .tv_nsec = SENSOR_READ_PERIOD_NANO_SECONDS};
    sensorPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ReadSensorTimerEventHandler, &readSensorPeriod);
    if (sensorPollTimer == NULL) {
        return ExitCode_Init_sensorPollTimer;
    }

#ifdef IOT_HUB_APPLICATION

    // Set up a timer to send telemetry.  SEND_TELEMETRY_PERIOD_SECONDS is defined in build_options.h
    static const struct timespec sendTelemetryPeriod = {.tv_sec = SEND_TELEMETRY_PERIOD_SECONDS,
                                                     .tv_nsec = SEND_TELEMETRY_PERIOD_NANO_SECONDS};
    telemetrytxIntervalr = CreateEventLoopPeriodicTimer(eventLoop, &SendTelemetryTimerEventHandler, &sendTelemetryPeriod);
    if (telemetrytxIntervalr == NULL) {
        return ExitCode_Init_TelemetrytxIntervalr;
    }


    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL) {
        return ExitCode_Init_AzureTimer;
    }

#endif // IOT_HUB_APPLICATION

#ifdef USE_IOT_CONNECT
    if (IoTConnectInit() != ExitCode_Success) {

        return ExitCode_Init_IoTCTimer;
    }
#endif


    // Initialize the i2c sensors
    lp_imu_initialize();

#ifdef M4_INTERCORE_COMMS
	//// ADC connection
	 	
	// Open connection to real-time capable application.
	sockFd = Application_Connect(rtAppComponentId);
	if (sockFd == -1) 
	{
		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
		Log_Debug("Real Time Core disabled or Component Id is not correct.\n");
		Log_Debug("The program will continue without showing light sensor data.\n");
		// Communication with RT core error
		RTCore_status = 1;
    }
	else
	{
		// Communication with RT core success
		RTCore_status = 0;
		// Set timeout, to handle case where real-time capable application does not respond.
		static const struct timeval recvTimeout = { .tv_sec = 5,.tv_usec = 0 };
		int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
		if (result == -1)
		{
			Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
			return -1;
		}

    	// Register handler for incoming messages from real-time capable application.
        rtAppEventReg = EventLoop_RegisterIo(eventLoop, sockFd, EventLoop_Input, SocketEventHandler, NULL);
        if (rtAppEventReg == NULL) {
            return ExitCode_Init_RegisterIo;
        }

        // Register one second timer to send a message to the real-time core.
    	static const struct timespec M4PollPeriod = { .tv_sec = M4_READ_PERIOD_SECONDS,.tv_nsec = M4_READ_PERIOD_NANO_SECONDS };
        M4PollTimer = CreateEventLoopPeriodicTimer(eventLoop, &M4TimerEventHandler, &M4PollPeriod);
        if (M4PollTimer == NULL) {
            return ExitCode_Init_Rt_PollTimer;
        }
    }

	//// end ADC Connection
#endif 

    return ExitCode_Success;
}

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

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    DisposeEventLoopTimer(sensorPollTimer);
    DisposeEventLoopTimer(telemetrytxIntervalr);

    // Call any direct method cleanup routines
    CleanupDirectMethods();

#ifdef M4_INTERCORE_COMMS    
    DisposeEventLoopTimer(M4txIntervalr);
#endif 
#ifdef OLED_SD1306
    DisposeEventLoopTimer(oledUpdateTimer);
#endif 
#ifdef IOT_HUB_APPLICATION    
    DisposeEventLoopTimer(azureTimer);
#endif // IOT_HUB_APPLICATION
    
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");
    CloseFdAndPrintError(buttonAgpioFd, "ButtonA Fd");
    CloseFdAndPrintError(buttonBgpioFd, "ButtonB Fd");

    // Close all the FD's associated with device twins
    deviceTwinCloseFDs();

    // Close the i2c interface
    void lp_imu_close(void);
}

#ifdef IOT_HUB_APPLICATION
/// <summary>
///     Callback when the Azure IoT connection state changes.
///     This can indicate that a new connection attempt has succeeded or failed.
///     It can also indicate than an existing connection has expired due to SAS token expiry.
/// </summary>
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                     IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                     void *userContextCallback)
{
    Log_Debug("Azure IoT connection status: %s\n", GetReasonString(reason));

    if (result != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;
        return;
    }

    iotHubClientAuthenticationState = IoTHubClientAuthenticationState_Authenticated;

    // Send static device twin properties when connection is established.
    TwinReportState("{\"manufacturer\":\"Avnet\",\"model\":\"Avnet Starter Kit\"}");

#ifdef USE_IOT_CONNECT
    // Call the routine that will send the hello message to IoTConnect
    IoTConnectConnectedToIoTHub();
#endif

	//#warning "If you need to upodate the version string do so here"
	checkAndUpdateDeviceTwin("versionString", "AvnetTemplate-V1", TYPE_STRING, false);

    // Send the current device twin properties.
    sendInitialDeviceTwinReportedProperties();

    // Read the current wifi configuration
    ReadWifiConfig(true);
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetUpAzureIoTHubClient(void)
{
    bool isClientSetupSuccessful = false;

    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
    }

    if ((connectionType == ConnectionType_Direct) || (connectionType == ConnectionType_IoTEdge)) {
        isClientSetupSuccessful = SetUpAzureIoTHubClientWithDaa();
    }  else if (connectionType == ConnectionType_DPS) {
        isClientSetupSuccessful = SetUpAzureIoTHubClientWithDps();
    }
#ifdef USE_PNP           
    else if (connectionType == ConnectionType_PnP){
        isClientSetupSuccessful = ProvisionWithDpsPnP();
    }
#endif 
    if (!isClientSetupSuccessful) {
        // If we fail to connect, reduce the polling frequency, starting at
        // AzureIoTMinReconnectPeriodSeconds and with a backoff up to
        // AzureIoTMaxReconnectPeriodSeconds
        if (azureIoTPollPeriodSeconds == AzureIoTDefaultPollPeriodSeconds) {
            azureIoTPollPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
        } else {
            azureIoTPollPeriodSeconds *= 2;
            if (azureIoTPollPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
                azureIoTPollPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
            }
        }

        struct timespec azureTelemetryPeriod = {azureIoTPollPeriodSeconds, 0};
        SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

        Log_Debug("ERROR: Failed to create IoTHub Handle - will retry in %i seconds.\n",
                  azureIoTPollPeriodSeconds);
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

    // Set client authentication state to initiated. This is done to indicate that
    // SetUpAzureIoTHubClient() has been called (and so should not be called again) while the
    // client is waiting for a response via the ConnectionStatusCallback().
    iotHubClientAuthenticationState = IoTHubClientAuthenticationState_AuthenticationInitiated;

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, DeviceTwinCallback, NULL);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, DeviceMethodCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle, ConnectionStatusCallback,
                                                      NULL);
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DAA
/// </summary>
static bool SetUpAzureIoTHubClientWithDaa(void)
{
    bool retVal = true;

    // Set up auth type
    int retError = iothub_security_init(IOTHUB_SECURITY_TYPE_X509);
    if (retError != 0) {
        Log_Debug("ERROR: iothub_security_init failed with error %d.\n", retError);
        return false;
    }

    // Create Azure Iot Hub client handle
    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(hostName, MQTT_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("IoTHubDeviceClient_LL_CreateFromDeviceAuth returned NULL.\n");
        retVal = false;
        goto cleanup;
    }

    // Enable DAA cert usage when x509 is invoked
    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, "SetDeviceId",
                                        &deviceIdForDaaCertUsage) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"SetDeviceId\".\n");
        retVal = false;
        goto cleanup;
    }

    if (connectionType == ConnectionType_IoTEdge) {
        // Provide the Azure IoT device client with the IoT Edge root
        // X509 CA certificate that was used to setup the Edge runtime.
        if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_TRUSTED_CERT,
                                            iotEdgeRootCACertContent) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"TrustedCerts\".\n");
            retVal = false;
            goto cleanup;
        }

        // Set the auto URL Encoder (recommended for MQTT).
        bool urlEncodeOn = true;
        if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE,
                                            &urlEncodeOn) != IOTHUB_CLIENT_OK) {
            Log_Debug(
                "ERROR: Failure setting Azure IoT Hub client option "
                "\"OPTION_AUTO_URL_ENCODE_DECODE\".\n");
            retVal = false;
            goto cleanup;
        }
    }

cleanup:
    iothub_security_deinit();

    return retVal;
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DPS
/// </summary>
static bool SetUpAzureIoTHubClientWithDps(void)
{
    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
                                                                          &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
              GetAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        return false;
    }

    return true;
}


/// <summary>
///     Converts the Azure IoT Hub connection status reason to a string.
/// </summary>
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char *GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

/// <summary>
///     Check the network status.
/// </summary>
bool IsConnectionReadyToSendTelemetry(void)
{
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(networkInterface, &status) != 0) {
        if (errno != EAGAIN) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                      strerror(errno));
            exitCode = ExitCode_InterfaceConnectionStatus_Failed;
            return false;
        }
        Log_Debug(
            "WARNING: Cannot send Azure IoT Hub telemetry because the networking stack isn't ready "
            "yet.\n");
        return false;
    }

    if ((status & Networking_InterfaceConnectionStatus_ConnectedToInternet) == 0) {
        Log_Debug(
            "WARNING: Cannot send Azure IoT Hub telemetry because the device is not connected to "
            "the internet.\n");
        return false;
    }

    return true;
}

/// <summary>
///     Sends telemetry to Azure IoT Hub
/// </summary>
void SendTelemetry(const char *jsonMessage, bool appendIoTConnectHeader)
{

    IOTHUB_MESSAGE_HANDLE messageHandle;

    // First check to see if we're connected to the IoT Hub, if not return!
    if (iotHubClientAuthenticationState != IoTHubClientAuthenticationState_Authenticated) {
        // AzureIoT client is not authenticated. Log a warning and return.
        Log_Debug("WARNING: Azure IoT Hub is not authenticated. Not sending telemetry.\n");
        return;
    }

    // Check whether the device is connected to the internet.
    if (IsConnectionReadyToSendTelemetry() == false) {
        return;
    }

#ifdef USE_IOT_CONNECT

    char *ioTConnectTelemetryBuffer;
    size_t ioTConnectMessageSize = strlen(jsonMessage) + IOTC_TELEMETRY_OVERHEAD;

    ioTConnectTelemetryBuffer = malloc(ioTConnectMessageSize);
    if (ioTConnectTelemetryBuffer == NULL) {
        exitCode = ExitCode_IoTCMalloc_Failed;
        return;
    }

    // If we don't need to append the IoTConnect header, then just send the original message
    // This sould be just the IoTConnect hello message
    if(!appendIoTConnectHeader){

        Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", jsonMessage);                                          
        messageHandle = IoTHubMessage_CreateFromString(jsonMessage);

    }
    else if (FormatTelemetryForIoTConnect(jsonMessage, ioTConnectTelemetryBuffer,
                                      ioTConnectMessageSize)) {

        Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", ioTConnectTelemetryBuffer);

        // Otherwise, set the message handle to use the modified message
        messageHandle = IoTHubMessage_CreateFromString(ioTConnectTelemetryBuffer);
    }
    else{

        Log_Debug("Not sending telemetry, not connected to IoTConnect!\n");

        // Free the memory
        free(ioTConnectTelemetryBuffer);
        return;
    }
#else

    Log_Debug("Sending Azure IoT Hub telemetry: %s.\n", jsonMessage);
    messageHandle = IoTHubMessage_CreateFromString(jsonMessage);

#endif

    // Make sure we created a valid message handle, if not cleanup and exit
    if (messageHandle == 0) {
        Log_Debug("ERROR: unable to create a new IoTHubMessage.\n");

#ifdef USE_IOT_CONNECT
        // Free the memory
        free(ioTConnectTelemetryBuffer);
#endif
        return;
    }
    
    // Attempt to send the message we created
    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendEventCallback,
                                             /*&callback_param*/ NULL) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure requesting IoTHubClient to send telemetry event.\n");
    } else {
        Log_Debug("INFO: IoTHubClient accepted the telemetry event for delivery.\n");
    }

    // Cleanup
    IoTHubMessage_Destroy(messageHandle);
#ifdef USE_IOT_CONNECT

    // Free the memory
    free(ioTConnectTelemetryBuffer);

#endif
}

/// <summary>
///     Callback invoked when the Azure IoT Hub send event request is processed.
/// </summary>
void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub send telemetry event callback: status code %d.\n", result);
}

/// <summary>
///     Callback invoked when the Device Twin report state request is processed by Azure IoT Hub
///     client.
/// </summary>
void ReportedStateCallback(int result, void *context)
{
    Log_Debug("INFO: Azure IoT Hub Device Twin reported state callback: status code %d.\n", result);
}

#endif // IOT_HUB_APPLICATION

/// <summary>
///     Check whether a given button has just been pressed/released.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if button state has changed, false otherwise</returns>
static bool ButtonStateChanged(int fd, GPIO_Value_Type *oldState)
{
    bool didButtonStateChange = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        didButtonStateChange = (newState != *oldState);
        *oldState = newState;
    }

    return didButtonStateChange;
}

#ifdef IOT_HUB_APPLICATION
/// <summary>
///     Read the certificate file and provide a null terminated string containing the certificate.
///     The function logs an error and returns an error code if it cannot allocate enough memory to
///     hold the certificate content.
/// </summary>
/// <returns>ExitCode_Success on success, any other exit code on error</returns>
static ExitCode ReadIoTEdgeCaCertContent(void)
{
    int certFd = -1;
    off_t fileSize = 0;

    certFd = Storage_OpenFileInImagePackage(iotEdgeRootCAPath);
    if (certFd == -1) {
        Log_Debug("ERROR: Storage_OpenFileInImagePackage failed with error code: %d (%s).\n", errno,
                  strerror(errno));
        return ExitCode_IoTEdgeRootCa_Open_Failed;
    }

    // Get the file size.
    fileSize = lseek(certFd, 0, SEEK_END);
    if (fileSize == -1) {
        Log_Debug("ERROR: lseek SEEK_END: %d (%s)\n", errno, strerror(errno));
        close(certFd);
        return ExitCode_IoTEdgeRootCa_LSeek_Failed;
    }

    // Reset the pointer to start of the file.
    if (lseek(certFd, 0, SEEK_SET) < 0) {
        Log_Debug("ERROR: lseek SEEK_SET: %d (%s)\n", errno, strerror(errno));
        close(certFd);
        return ExitCode_IoTEdgeRootCa_LSeek_Failed;
    }

    if (fileSize == 0) {
        Log_Debug("File size invalid for %s\r\n", iotEdgeRootCAPath);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileSize_Invalid;
    }

    if (fileSize > MAX_ROOT_CA_CERT_CONTENT_SIZE) {
        Log_Debug("File size for %s is %lld bytes. Max file size supported is %d bytes.\r\n",
                  iotEdgeRootCAPath, fileSize, MAX_ROOT_CA_CERT_CONTENT_SIZE);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileSize_TooLarge;
    }

    // Copy the file into the buffer.
    ssize_t read_size = read(certFd, &iotEdgeRootCACertContent, (size_t)fileSize);
    if (read_size != (size_t)fileSize) {
        Log_Debug("Error reading file %s\r\n", iotEdgeRootCAPath);
        close(certFd);
        return ExitCode_IoTEdgeRootCa_FileRead_Failed;
    }

    // Add the null terminator at the end.
    iotEdgeRootCACertContent[fileSize] = '\0';

    close(certFd);
    return ExitCode_Success;
}

/// <summary>
///     Enqueues a report containing Device Twin reported properties. The report is not sent
///     immediately, but it is sent on the next invocation of IoTHubDeviceClient_LL_DoWork().
/// </summary>
void TwinReportState(const char *jsonState)
{
    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: Azure IoT Hub client not initialized.\n");
    } else {
        if (IoTHubDeviceClient_LL_SendReportedState(
                iothubClientHandle, (const unsigned char *)jsonState, strlen(jsonState),
                ReportedStateCallback, NULL) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: Azure IoT Hub client error when reporting state '%s'.\n", jsonState);
        } else {
            Log_Debug("INFO: Azure IoT Hub client accepted request to report state '%s'.\n",
                      jsonState);
        }
    }
}

#ifdef USE_PNP
/// <summary>
///     DPS provisioning callback with status
/// </summary>
static void RegisterDeviceCallback(PROV_DEVICE_RESULT registerResult, const char* callbackHubUri, const char* deviceId, void* userContext)
{
    dpsRegisterStatus = registerResult;

	if (registerResult == PROV_DEVICE_RESULT_OK && callbackHubUri != NULL) {

		size_t uriSize = strlen(callbackHubUri) + 1; // +1 for NULL string termination

		iotHubUri = (char*)malloc(uriSize);

		if (iotHubUri == NULL) {
			Log_Debug("ERROR: IoT Hub URI malloc failed.\n");
		}
		else {
			strncpy(iotHubUri, callbackHubUri, uriSize);
		}
	}
}

bool lp_isNetworkReady(void) {
	bool isNetworkReady = false;
	if (Networking_IsNetworkingReady(&isNetworkReady) != 0) {
		Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
	}
	else {
		if (!isNetworkReady) {
			Log_Debug("\nNetwork not ready.\nFrom azure sphere command prompt, run azsphere device wifi show-status\n\n");
		}
	}

	return isNetworkReady;
}

bool lp_isDeviceAuthReady(void) {
	// Verifies authentication is ready on device
	bool currentAppDeviceAuthReady = false;
	if (Application_IsDeviceAuthReady(&currentAppDeviceAuthReady) != 0) {
		Log_Debug("ERROR: Application_IsDeviceAuthReady: %d (%s)\n", errno, strerror(errno));
	}
	else {
		if (!currentAppDeviceAuthReady) {
			Log_Debug("ERROR: Current Application not Device Auth Ready\n");
		}
	}

	return currentAppDeviceAuthReady;
}


/// <summary>
///     Provision with DPS and assign IoT Plug and Play Model ID
/// </summary>
static bool ProvisionWithDpsPnP(void)
{
	PROV_DEVICE_LL_HANDLE prov_handle = NULL;
	PROV_DEVICE_RESULT prov_result;
	bool result = false;
	char* dtdlBuffer = NULL;
	int deviceIdForDaaCertUsage = 0;  // set DaaCertUsage to false

	if (!lp_isNetworkReady() || !lp_isDeviceAuthReady()) {
		return false;
	}

//    #define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:avnetoob;1"	// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
    const char* _deviceTwinModelId = IOT_PLUG_AND_PLAY_MODEL_ID;

	if (_deviceTwinModelId != NULL && strlen(_deviceTwinModelId) > 0)
	{
		size_t modelIdLen = 20; // allow for JSON format "{\"modelId\":\"%s\"}", 14 char, plus null and a couple of extra :)
		modelIdLen += strlen(_deviceTwinModelId); // allow for twin property name in JSON response

		dtdlBuffer = (char*)malloc(modelIdLen);
		if (dtdlBuffer == NULL) {
			Log_Debug("ERROR: PnP Model ID malloc failed.\n");
			goto cleanup;
		}

		int len = snprintf(dtdlBuffer, modelIdLen, "{\"modelId\":\"%s\"}", _deviceTwinModelId);
		if (len < 0 || len >= modelIdLen) {
			Log_Debug("ERROR: Cannot write Model ID to buffer.\n");
			goto cleanup;
		}
	}

	// Initiate security with X509 Certificate
	if (prov_dev_security_init(SECURE_DEVICE_TYPE_X509) != 0) {
		Log_Debug("ERROR: Failed to initiate X509 Certificate security\n");
		goto cleanup;
	}

	// Create Provisioning Client for communication with DPS using MQTT protocol
	if ((prov_handle = Prov_Device_LL_Create(dpsUrl, scopeId, Prov_Device_MQTT_Protocol)) == NULL) {
		Log_Debug("ERROR: Failed to create Provisioning Client\n");
		goto cleanup;
	}

	// Sets Device ID on Provisioning Client
	if ((prov_result = Prov_Device_LL_SetOption(prov_handle, "SetDeviceId", &deviceIdForDaaCertUsage)) != PROV_DEVICE_RESULT_OK) {
		Log_Debug("ERROR: Failed to set Device ID in Provisioning Client, error=%d\n", prov_result);
		goto cleanup;
	}

	// Sets Model ID provisioning data
	if (dtdlBuffer != NULL) {
		if ((prov_result = Prov_Device_LL_Set_Provisioning_Payload(prov_handle, dtdlBuffer)) != PROV_DEVICE_RESULT_OK) {
			Log_Debug("Error: Failed to set Model ID in Provisioning Client, error=%d\n", prov_result);
			goto cleanup;
		}
	}

	// Sets the callback function for device registration
	if ((prov_result = Prov_Device_LL_Register_Device(prov_handle, RegisterDeviceCallback, NULL, NULL, NULL)) != PROV_DEVICE_RESULT_OK) {
		Log_Debug("ERROR: Failed to set callback function for device registration, error=%d\n", prov_result);
		goto cleanup;
	}

	// Begin provisioning device with DPS
	// Initiates timer to prevent timing out
	static const long timeoutMs = 60000; // aloow up to 60 seconds before timeout
	static const long workDelayMs = 25;
	const struct timespec sleepTime = { .tv_sec = 0, .tv_nsec = workDelayMs * 1000 * 1000 };
	long time_elapsed = 0;

	dpsRegisterStatus = PROV_DEVICE_REG_HUB_NOT_SPECIFIED;

	while (dpsRegisterStatus != PROV_DEVICE_RESULT_OK && time_elapsed < timeoutMs) {
		Prov_Device_LL_DoWork(prov_handle);
		nanosleep(&sleepTime, NULL);
		time_elapsed += workDelayMs;
	}

	if (dpsRegisterStatus != PROV_DEVICE_RESULT_OK) {
		Log_Debug("ERROR: Failed to register device with provisioning service\n");
		goto cleanup;
	}

	if ((iothubClientHandle = IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(iotHubUri, MQTT_Protocol)) == NULL) {
		Log_Debug("ERROR: Failed to create client IoT Hub Client Handle\n");
		goto cleanup;
	}

	// IOTHUB_CLIENT_RESULT iothub_result 
	int deviceId = 1;
	if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, "SetDeviceId", &deviceId) != IOTHUB_CLIENT_OK) {
		IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
		iothubClientHandle = NULL;
		Log_Debug("ERROR: Failed to set Device ID on IoT Hub Client\n");
		goto cleanup;
	}

	// Sets auto URL encoding on IoT Hub Client
	bool urlAutoEncodeDecode = true;
	if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode) != IOTHUB_CLIENT_OK) {
		Log_Debug("ERROR: Failed to set auto Url encode option on IoT Hub Client\n");
		goto cleanup;
	}

	if (dtdlBuffer != NULL) {
		if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_MODEL_ID, _deviceTwinModelId) != IOTHUB_CLIENT_OK)
		{
			Log_Debug("ERROR: failure setting option \"%s\"\n", OPTION_MODEL_ID);
			goto cleanup;
		}
	}

	result = true;

cleanup:
	if (dtdlBuffer != NULL) {
		free(dtdlBuffer);
		dtdlBuffer = NULL;
	}

	if (iotHubUri != NULL) {
		free(iotHubUri);
		iotHubUri = NULL;
	}

	if (prov_handle != NULL) {
		Prov_Device_LL_Destroy(prov_handle);
	}

	prov_dev_security_deinit();
	return result;
}

#endif // USE_PNP
#endif // IOT_HUB_APPLICATION

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
   			// Note that we send up this data to Azure if it changes, but the IoT Central Properties elements only 
   			// show the data that was currenet when the device first connected to Azure.
   			checkAndUpdateDeviceTwin("ssid", &network_data.SSID, TYPE_STRING, false);
   			checkAndUpdateDeviceTwin("freq", &network_data.frequency_MHz, TYPE_INT, false);
   			checkAndUpdateDeviceTwin("bssid", &bssid, TYPE_STRING, false);
            
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

#ifdef M4_INTERCORE_COMMS
//// ADC connection

/// <summary>
///     Handle socket event by reading incoming data from real-time capable application.
/// </summary>
//static void SocketEventHandler(EventData *eventData)
static void SocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
	// Read response from real-time capable application.
	char rxBuf[32];
	union Analog_data
	{
		uint32_t u32;
		uint8_t u8[4];
	} analog_data;

	int bytesReceived = recv(sockFd, rxBuf, sizeof(rxBuf), 0);

	if (bytesReceived == -1) {
		Log_Debug("ERROR: Unable to receive message: %d (%s)\n", errno, strerror(errno));
		exitCode = ExitCode_Read_RT_Socket;
	}

	// Copy data from Rx buffer to analog_data union
	for (int i = 0; i < sizeof(analog_data); i++)
	{
		analog_data.u8[i] = rxBuf[i];
	}

	// get voltage (2.5*adc_reading/4096)
	// divide by 3650 (3.65 kohm) to get current (A)
	// multiply by 1000000 to get uA
	// divide by 0.1428 to get Lux (based on fluorescent light Fig. 1 datasheet)
	// divide by 0.5 to get Lux (based on incandescent light Fig. 1 datasheet)
	// We can simplify the factors, but for demostration purpose it's OK
	light_sensor = (float)(analog_data.u32*2.5/4095)*1000000 / (float)(3650*0.1428);

	Log_Debug("Received %d bytes. ", bytesReceived);

	Log_Debug("\n");	
}

/// <summary>
///     Handle send timer event by writing data to the real-time capable application.
/// </summary>
static void M4TimerEventHandler(EventLoopTimer *timer)
{

        if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_RT_Timer_Consume;
        return;
    }

	SendMessageToRTCore();
}

/// <summary>
///     Helper function for M4TimerEventHandler sends message to real-time capable application.
/// </summary>
static void SendMessageToRTCore(void)
{
	static int iter = 0;

	// Send "Read-ADC-%d" message to real-time capable application.
	static char txMessage[32];
	sprintf(txMessage, "Read-ADC-%d", iter++);
	Log_Debug("Sending: %s\n", txMessage);

	int bytesSent = send(sockFd, txMessage, strlen(txMessage), 0);
	if (bytesSent == -1)
	{
		Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_Write_RT_Socket;
		return;
	}
}

//// end ADC connection
#endif 

/// <summary>
///     Reboot the device.
/// </summary>
static void TriggerReboot(void)
{
    // Reboot the system
    int result = PowerManagement_ForceSystemReboot();
    if (result != 0) {
        Log_Debug("Error PowerManagement_ForceSystemReboot: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_UpdateCallback_Reboot;
    }
}

