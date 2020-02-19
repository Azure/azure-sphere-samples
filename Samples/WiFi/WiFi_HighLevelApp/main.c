/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the networking
// interfaces. Each press of BUTTON_1 will advance through a cycle that adds, disables,
// enables, and deletes an example network. BUTTON_2 will show the device network status,
// list the stored networks, trigger a network scan.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - wificonfig (for configuring the example Wi-Fi connection)
// - networking (for reading the device's overall network state)
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/wificonfig.h>
#include <applibs/networking.h>
#include <applibs/log.h>

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

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They they must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_RetrieveNetworks_GetCount = 1,
    ExitCode_RetreiveNetworks_GetStored = 2,

    ExitCode_ConfAddState_WrongSecType = 3,
    ExitCode_ConfAddState_AddNetwork = 4,
    ExitCode_ConfAddState_SetSecType = 5,
    ExitCode_ConfAddState_SetPsk = 6,
    ExitCode_ConfAddState_SetSsid = 7,
    ExitCode_ConfAddState_PersistConfig = 8,

    ExitCode_EnableState_SetNetworkEnabled = 9,

    ExitCode_DisableState_SetNetworkEnabled = 10,

    ExitCode_DeleteState_ForgetNetworkById = 11,
    ExitCode_DeleteState_PersistConfig = 12,

    ExitCode_CheckNetworkReady_IsReady = 13,
    ExitCode_CheckStatus_GetCurrentNetwork = 14,

    ExitCode_OutputStored_RetrieveNetworks = 15,

    ExitCode_OutputScanned_TriggerScan = 16,
    ExitCode_OutputScanned_GetScanned = 17,

    ExitCode_OutputNetworks_RetrieveNetworks = 18,

    ExitCode_IsButtonPressed_GetValue = 19,

    ExitCode_ButtonTimerHandler_Consume = 20,

    ExitCode_Init_EventLoop = 21,
    ExitCode_Init_SampleButton = 22,
    ExitCode_Init_StatusButton = 23,
    ExitCode_Init_ButtonTimer = 24,

    ExitCode_Main_EventLoopFail = 25
} ExitCode;

// The MT3620 currently handles a maximum of 37 stored wifi networks.
static const unsigned int MAX_NUMBER_STORED_NETWORKS = 37;

// Network configuration: Configure the variables with your Wpa2 network information
static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";
static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;
static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";

// Compute the SSID length based on the configured sampleNetworkSsid
static uint8_t sampleNetworkSsidLength =
    sizeof(sampleNetworkSsid) / sizeof(sampleNetworkSsid[0]) - 1;

// Array used to print the network security type as a string
static const char *securityTypeToString[] = {"Unknown", "Open", "WPA2/PSK"};

// File descriptors - initialized to invalid value
static int changeNetworkConfigButtonGpioFd = -1;
static int showNetworkStatusButtonGpioFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// Button state variables
static GPIO_Value_Type changeNetworkConfigButtonState = GPIO_Value_High;
static GPIO_Value_Type showNetworkStatusButtonState = GPIO_Value_High;

static int sampleStoredNetworkId = -1;

static void TerminationHandler(int signalNumber);
static void StateStatusOutputHelper(const char *currentStateMessage, const char *nextStateMessage,
                                    bool statusIsSuccessful);
static bool IsSameStoredWifiNetwork(const WifiConfig_StoredNetwork *target);
static bool IsSameScannedWifiNetwork(const WifiConfig_ScannedNetwork *target,
                                     const WifiConfig_ScannedNetwork *source);
static ExitCode WifiRetrieveStoredNetworks(ssize_t *numberOfNetworksStored,
                                           WifiConfig_StoredNetwork *storedNetworksArray);

int CompareSsid(const void *network1, const void *network2);
static void SortAndDeduplicateAvailableNetworks(
    const WifiConfig_ScannedNetwork *scannedNetworksArray, size_t numberOfScannedNetworks);
static ExitCode CheckNetworkReady(void);
static ExitCode CheckCurrentWifiNetworkStatus(void);
static ExitCode OutputStoredWifiNetworks(void);
static ExitCode OutputScannedWifiNetworks(void);
static void ShowDeviceNetworkStatus(void);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void ButtonEventTimeHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// Available states
static void WifiNetworkConfigureAndAddState(void);
static void WifiNetworkEnableState(void);
static void WifiNetworkDisableState(void);
static void WifiNetworkDeleteState(void);

// Pointer to the next state
typedef void (*NextStateFunctionPtr)(void);

// Each Button_1 press will advance the state
static NextStateFunctionPtr nextStateFunction = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_Success;
}

/// <summary>
///     Ouput the name and status of the current state and the name of the next state triggered
///     by pressing BUTTON_1.
/// </summary>
static void StateStatusOutputHelper(const char *currentStateMessage, const char *nextStateMessage,
                                    bool statusIsSuccessful)
{
    Log_Debug(
        "\nFinished %s network with status: %s. By pressing BUTTON_1 the network will be %s.\n",
        currentStateMessage, statusIsSuccessful ? "SUCCESS" : "FAILED", nextStateMessage);
}

/// <summary>
///     Checks if the given stored network is the same as the one specified in the configuration
///     above.
/// </summary>
/// <param name="target">Target network used for comparison</param>
/// <returns>True if is the same access point, false otherwise</returns>
static bool IsSameStoredWifiNetwork(const WifiConfig_StoredNetwork *target)
{
    if (target->security == sampleNetworkSecurityType &&
        target->ssidLength == sampleNetworkSsidLength) {
        return 0 == memcmp(target->ssid, sampleNetworkSsid, sampleNetworkSsidLength);
    }
    return false;
}

/// <summary>
///     Checks if the given scanned network is the same as the one specified in the configuration
///     above.
/// </summary>
/// <param name="target">Target network used for comparison</param>
/// <param name="source">Source network used for comparison</param>
/// <returns>True if is the same access point, false otherwise</returns>
static bool IsSameScannedWifiNetwork(const WifiConfig_ScannedNetwork *target,
                                     const WifiConfig_ScannedNetwork *source)
{
    if (target->security == source->security && target->ssidLength == source->ssidLength) {
        return 0 == memcmp(target->ssid, source->ssid, target->ssidLength);
    }
    return false;
}

/// <summary>
///     Retrieves the stored networks on the device.
/// </summary>
/// <param name="numberOfNetworksStored">Output param used to retrieve the number of stored
/// networks on the device</param>
/// <param name="storedNetworksArray">Output param used to maintain the stored networks on the
/// device.</param>
/// <returns>Returns 0 in case of success, -1 otherwise</returns>
static ExitCode WifiRetrieveStoredNetworks(ssize_t *numberOfNetworksStored,
                                           WifiConfig_StoredNetwork *storedNetworksArray)
{
    ssize_t temporaryNumberOfNetworks = WifiConfig_GetStoredNetworkCount();
    if (temporaryNumberOfNetworks < 0) {
        Log_Debug("ERROR: WifiConfig_GetStoredNetworkCount failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_RetrieveNetworks_GetCount;
    }

    assert(temporaryNumberOfNetworks <= MAX_NUMBER_STORED_NETWORKS);

    if (temporaryNumberOfNetworks == 0) {
        *numberOfNetworksStored = 0;
        return ExitCode_Success;
    }

    WifiConfig_StoredNetwork temporaryStoredNetworksArray[temporaryNumberOfNetworks];
    temporaryNumberOfNetworks = WifiConfig_GetStoredNetworks(temporaryStoredNetworksArray,
                                                             (size_t)temporaryNumberOfNetworks);
    if (temporaryNumberOfNetworks < 0) {
        Log_Debug("ERROR: WifiConfig_GetStoredNetworks failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_RetreiveNetworks_GetStored;
    }
    for (size_t i = 0; i < temporaryNumberOfNetworks; ++i) {
        storedNetworksArray[i] = temporaryStoredNetworksArray[i];
    }

    *numberOfNetworksStored = temporaryNumberOfNetworks;

    return ExitCode_Success;
}

/// <summary>
///     Configures and stores a new network based on the SSID, network security type and the psk
///     provided and saves the configuration.
/// </summary>
static void WifiNetworkConfigureAndAddState(void)
{
    assert(sampleNetworkSsidLength < WIFICONFIG_SSID_MAX_LENGTH);

    if (sampleNetworkSecurityType != WifiConfig_Security_Open &&
        sampleNetworkSecurityType != WifiConfig_Security_Wpa2_Psk) {
        Log_Debug(
            "ERROR: sampleNetworkSecurityType should be set to WifiConfig_Security_Open"
            " or WifiConfig_Security_Wpa2_Psk.\n");
        exitCode = ExitCode_ConfAddState_WrongSecType;
        return;
    }

    ssize_t numberOfNetworksStored;
    WifiConfig_StoredNetwork storedNetworksArray[MAX_NUMBER_STORED_NETWORKS];
    ExitCode localExitCode =
        WifiRetrieveStoredNetworks(&numberOfNetworksStored, storedNetworksArray);
    if (localExitCode != ExitCode_Success) {
        exitCode = localExitCode;
        return;
    }

    // If the ssid is stored, move to the next state
    // otherwise continue configuring the new network
    for (size_t i = 0; i < numberOfNetworksStored; ++i) {
        if (IsSameStoredWifiNetwork(&storedNetworksArray[i])) {
            sampleStoredNetworkId = (int)i;
            nextStateFunction = WifiNetworkEnableState;
            StateStatusOutputHelper("storing the existing", "enabled", true);
            return;
        }
    }

    sampleStoredNetworkId = WifiConfig_AddNetwork();
    if (sampleStoredNetworkId < 0) {
        Log_Debug("ERROR: WifiConfig_AddNetwork failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ConfAddState_AddNetwork;
        return;
    }

    int result = WifiConfig_SetSecurityType(sampleStoredNetworkId, sampleNetworkSecurityType);
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_SetSecurityType failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ConfAddState_SetSecType;
        return;
    }

    // if the network security is Wpa2_Psk, set the Psk
    if (sampleNetworkSecurityType == WifiConfig_Security_Wpa2_Psk) {
        result = WifiConfig_SetPSK(sampleStoredNetworkId, sampleNetworkPsk,
                                   strnlen(sampleNetworkPsk, WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE));
        if (result < 0) {
            Log_Debug("ERROR: WifiConfig_SetPSK failed: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_ConfAddState_SetPsk;
            return;
        }
    }

    result = WifiConfig_SetSSID(sampleStoredNetworkId, sampleNetworkSsid, sampleNetworkSsidLength);
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_SetSSID failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ConfAddState_SetSsid;
        return;
    }

    result = WifiConfig_PersistConfig();
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_PersistConfig failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ConfAddState_PersistConfig;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkEnableState;
    StateStatusOutputHelper("configuring and adding the", "enabled", true);
}

/// <summary>
///     Enables the configured network.
/// </summary>
static void WifiNetworkEnableState(void)
{
    int result = WifiConfig_SetNetworkEnabled(sampleStoredNetworkId, true);
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_SetNetworkEnabled failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_EnableState_SetNetworkEnabled;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkDisableState;
    StateStatusOutputHelper("enabling the", "disabled", true);
}

/// <summary>
///     Disables the configured network.
/// </summary>
static void WifiNetworkDisableState(void)
{
    int result = WifiConfig_SetNetworkEnabled(sampleStoredNetworkId, false);
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_SetNetworkEnabled failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_DisableState_SetNetworkEnabled;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkDeleteState;
    StateStatusOutputHelper("disabling the", "deleted", true);
}

/// <summary>
///     Deletes the configured network and saves the configuration.
/// </summary>
static void WifiNetworkDeleteState(void)
{
    int result = WifiConfig_ForgetNetworkById(sampleStoredNetworkId);
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_ForgetNetworkById failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_DeleteState_ForgetNetworkById;
        return;
    }

    result = WifiConfig_PersistConfig();
    if (result < 0) {
        Log_Debug("ERROR: WifiConfig_PersistConfig failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_DeleteState_PersistConfig;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkConfigureAndAddState;
    StateStatusOutputHelper("deleting the", "configured and added", true);
}

/// <summary>
///     Comparison function used to sort available networks based on SSID.
/// </summary>
/// <param name="network1">The first scanned network to compare</param>
/// <param name="network2">The second scanned network to compare</param>
int CompareSsid(const void *network1, const void *network2)
{
    const WifiConfig_ScannedNetwork *scannedNetwork1 = (const WifiConfig_ScannedNetwork *)network1;
    const WifiConfig_ScannedNetwork *scannedNetwork2 = (const WifiConfig_ScannedNetwork *)network2;
    size_t minSsidLength = 0;
    if (scannedNetwork1->ssidLength > scannedNetwork2->ssidLength) {
        minSsidLength = scannedNetwork2->ssidLength;
    } else {
        minSsidLength = scannedNetwork1->ssidLength;
    }

    return memcmp(scannedNetwork1->ssid, scannedNetwork2->ssid, minSsidLength);
}

/// <summary>
///     Sorts the available scanned networks based on their RSSI signals and outputs the SSID and
///     the RSSI signal.
/// </summary>
/// <param name="scannedNetworksArray">An array which contains the scanned networks</param>
/// <param name="numberOfScannedNetworks">The size of the array</param>
static void SortAndDeduplicateAvailableNetworks(
    const WifiConfig_ScannedNetwork *scannedNetworksArray, size_t numberOfScannedNetworks)
{
    // sort the array based on the SSID
    qsort((void *)scannedNetworksArray, numberOfScannedNetworks, sizeof(WifiConfig_ScannedNetwork),
          CompareSsid);

    WifiConfig_ScannedNetwork deduplicatedScannedNetworks[numberOfScannedNetworks];
    deduplicatedScannedNetworks[0] = scannedNetworksArray[0];
    unsigned int j = 0;
    unsigned int i = 1;

    // iterate over the array and keep the SSID with the highest RSSI signal
    for (i = 1; i < numberOfScannedNetworks; ++i) {
        if (!IsSameScannedWifiNetwork(&deduplicatedScannedNetworks[j], &scannedNetworksArray[i])) {
            j++;
            deduplicatedScannedNetworks[j] = scannedNetworksArray[i];
        } else if (deduplicatedScannedNetworks[j].signalRssi < scannedNetworksArray[i].signalRssi) {
            deduplicatedScannedNetworks[j] = scannedNetworksArray[i];
        }
    }
    size_t numberOfDeduplicatedNetworks = j + 1;

    Log_Debug("INFO: Available Wi-Fi networks:\n");
    for (i = 0; i < numberOfDeduplicatedNetworks; ++i) {
        for (j = 0; j < deduplicatedScannedNetworks[i].ssidLength; ++j) {
            Log_Debug("%c", isprint(deduplicatedScannedNetworks[i].ssid[j])
                                ? deduplicatedScannedNetworks[i].ssid[j]
                                : '.');
        }
        assert(deduplicatedScannedNetworks[i].security < 3);
        Log_Debug(" : %s : %d dB\n", securityTypeToString[deduplicatedScannedNetworks[i].security],
                  deduplicatedScannedNetworks[i].signalRssi);
    }
}

/// <summary>
///     Checks if the device is connected to any Wi-Fi networks.
/// </summary>
/// <returns>0 in case of success, any other value in case of failure</returns>
static ExitCode CheckNetworkReady(void)
{
    bool isNetworkReady;
    int result = Networking_IsNetworkingReady(&isNetworkReady);
    if (result != 0) {
        Log_Debug("\nERROR: Networking_IsNetworkingReady failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_CheckNetworkReady_IsReady;
    }
    if (!isNetworkReady) {
        Log_Debug("INFO: Internet connectivity is not available.\n");
    } else {
        Log_Debug("INFO: Internet connectivity is available.\n");
    }

    return ExitCode_Success;
}

/// <summary>
///     Checks if the current Wi-Fi network is enabled, connected and outputs its SSID,
///     RSSI signal and security type.
/// </summary>
/// <returns>0 in case of success, any other value in case of failure</returns>
static ExitCode CheckCurrentWifiNetworkStatus(void)
{
    // Check the current Wi-Fi network status
    WifiConfig_ConnectedNetwork connectedNetwork;
    int result = WifiConfig_GetCurrentNetwork(&connectedNetwork);
    if (result != 0 && errno != ENODATA) {
        Log_Debug("\nERROR: WifiConfig_GetCurrentNetwork failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_CheckStatus_GetCurrentNetwork;
    }

    if (result != 0 && errno == ENODATA) {
        Log_Debug("INFO: The device is not connected to any Wi-Fi networks.\n");
        return ExitCode_Success;
    }

    Log_Debug("INFO: The device is connected to: ");
    for (unsigned int i = 0; i < connectedNetwork.ssidLength; ++i) {
        Log_Debug("%c", isprint(connectedNetwork.ssid[i]) ? connectedNetwork.ssid[i] : '.');
    }
    assert(connectedNetwork.security < 3);
    Log_Debug(" : %s : %d dB\n", securityTypeToString[connectedNetwork.security],
              connectedNetwork.signalRssi);

    return ExitCode_Success;
}

/// <summary>
///    Outputs the stored Wi-Fi networks.
/// </summary>
/// <returns>0 in case of success, any other value in case of failure</returns>
static ExitCode OutputStoredWifiNetworks(void)
{
    ssize_t numberOfNetworksStored;
    WifiConfig_StoredNetwork storedNetworksArray[MAX_NUMBER_STORED_NETWORKS];
    ExitCode localExitCode =
        WifiRetrieveStoredNetworks(&numberOfNetworksStored, storedNetworksArray);
    if (localExitCode != ExitCode_Success) {
        exitCode = ExitCode_OutputStored_RetrieveNetworks;
        return localExitCode;
    }

    if (numberOfNetworksStored == 0) {
        return ExitCode_Success;
    }

    Log_Debug("INFO: Stored Wi-Fi networks:\n");
    for (unsigned int i = 0; i < numberOfNetworksStored; ++i) {
        for (unsigned int j = 0; j < storedNetworksArray[i].ssidLength; ++j) {
            Log_Debug("%c", isprint(storedNetworksArray[i].ssid[j]) ? storedNetworksArray[i].ssid[j]
                                                                    : '.');
        }
        assert(storedNetworksArray[i].security < 3);
        Log_Debug(" : %s : %s : %s\n", securityTypeToString[storedNetworksArray[i].security],
                  storedNetworksArray[i].isEnabled ? "Enabled" : "Disabled",
                  storedNetworksArray[i].isConnected ? "Connected" : "Disconnected");
    }

    return ExitCode_Success;
}

/// <summary>
///     Triggers a Wi-Fi network scan, stores the available networks, deduplicates them and outputs
///     the SSID of the available networks sorted and deduplicated based on their SSID.
/// </summary>
/// <returns>0 in case of success, any other value in case of failure</returns>
static ExitCode OutputScannedWifiNetworks(void)
{
    // Check the available Wi-Fi networks
    ssize_t numberOfNetworks = WifiConfig_TriggerScanAndGetScannedNetworkCount();
    if (numberOfNetworks < 0) {
        Log_Debug("ERROR: WifiConfig_TriggerScanAndGetScannedNetworkCount failed: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_OutputScanned_TriggerScan;
    }

    if (numberOfNetworks == 0) {
        Log_Debug("INFO: Couldn't find any available Wi-Fi networks\n");
        return ExitCode_Success;
    }

    WifiConfig_ScannedNetwork scannedNetworksArray[numberOfNetworks];
    ssize_t numberOfScannedNetworks =
        WifiConfig_GetScannedNetworks(scannedNetworksArray, (size_t)numberOfNetworks);
    if (numberOfScannedNetworks < 0) {
        Log_Debug("ERROR: WifiConfig_GetScannedNetworks failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_OutputScanned_GetScanned;
    }

    SortAndDeduplicateAvailableNetworks(scannedNetworksArray, (size_t)numberOfScannedNetworks);

    return ExitCode_Success;
}

/// <summary>
///     Checks if the device is connected to any Wi-Fi networks. Outputs the stored Wi-Fi networks.
///     Triggers a Wi-Fi network scan, and outputs the available Wi-Fi networks.
/// </summary>
static void ShowDeviceNetworkStatus(void)
{
    ExitCode localExitCode = CheckNetworkReady();

    if (localExitCode == ExitCode_Success) {
        localExitCode = CheckCurrentWifiNetworkStatus();
    }

    if (localExitCode == ExitCode_Success) {
        localExitCode = OutputStoredWifiNetworks();
    }

    if (localExitCode == ExitCode_Success) {
        localExitCode = OutputScannedWifiNetworks();
    }

    exitCode = localExitCode;
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>True if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
/// Button timer event:  Check the status of both buttons.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
static void ButtonEventTimeHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimerHandler_Consume;
        return;
    }

    // Check if BUTTON_1 was pressed
    bool isButtonPressed =
        IsButtonPressed(changeNetworkConfigButtonGpioFd, &changeNetworkConfigButtonState);
    if (isButtonPressed) {
        nextStateFunction();
    }

    // Check if BUTTON_2 was pressed
    isButtonPressed = IsButtonPressed(showNetworkStatusButtonGpioFd, &showNetworkStatusButtonState);
    if (isButtonPressed) {
        ShowDeviceNetworkStatus();
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>ExitCode_Success if all resources were allocated successfully; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
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

    // Open button GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    changeNetworkConfigButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (changeNetworkConfigButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1 GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_SampleButton;
    }

    // By pressing BUTTON_1 the WifiNetworkConfigureAndAddState will be called
    nextStateFunction = WifiNetworkConfigureAndAddState;

    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    showNetworkStatusButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (showNetworkStatusButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2 GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_StatusButton;
    }

    static const struct timespec buttonPressCheckPeriod100Ms = {.tv_sec = 0,
                                                                .tv_nsec = 100 * 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonEventTimeHandler,
                                                   &buttonPressCheckPeriod100Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonTimer;
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
    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("\nClosing file descriptors.\n");
    CloseFdAndPrintError(changeNetworkConfigButtonGpioFd, "Button1Gpio");
    CloseFdAndPrintError(showNetworkStatusButtonGpioFd, "Button2Gpio");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Wi-Fi application starting.\n");
    Log_Debug(
        "Each press of BUTTON_1 will advance through a cycle that adds, enables,"
        " disables, and deletes a Wi-Fi example network.\n");
    Log_Debug(
        "Press BUTTON_2 to check the device a Wi-Fi network configuration, trigger a Wi-Fi network"
        " scan and print a deduplicated list of available Wi-Fi networks.\n");

    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens.
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