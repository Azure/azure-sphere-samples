/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the networking
// interfaces. Each press of SAMPLE_BUTTON_1 will advance through a cycle that adds, disables,
// enables, duplicates, and deletes an example network. SAMPLE_BUTTON_2 will show the device
// network status, the network diagnostics, list the stored networks, and trigger a
// network scan.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - wificonfig (for configuring the example Wi-Fi connection)
// - networking (for reading the device's overall network state)
// - log (displays messages in the Device Output window during debugging)
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

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_RetrieveNetworks_GetCount = 1,
    ExitCode_RetrieveNetworks_GetStored = 2,

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

    ExitCode_IsNetworkingReady_Failed = 13,
    ExitCode_CheckStatus_GetCurrentNetwork = 14,

    ExitCode_OutputStored_RetrieveNetworks = 15,

    ExitCode_OutputScanned_TriggerScan = 16,
    ExitCode_OutputScanned_GetScanned = 17,

    ExitCode_IsButtonPressed_GetValue = 19,

    ExitCode_ButtonTimerHandler_Consume = 20,

    ExitCode_Init_EventLoop = 21,
    ExitCode_Init_SampleButton = 22,
    ExitCode_Init_StatusButton = 23,
    ExitCode_Init_ButtonTimer = 24,

    ExitCode_Main_EventLoopFail = 25,

    ExitCode_RetrieveNetworkIdByConfigName_GetNetworkIdByConfigName = 18,
    ExitCode_SetNetworkConfigName_SetConfigName = 26,
    ExitCode_RetrieveNetworkDiagnostics_GetNetworkIdByConfigName = 27,
    ExitCode_RetrieveNetworkDiag_GetNetworkDiagnostics = 28,
    ExitCode_ConfEapTls_SetRootCACertStoreIdentifier = 29,
    ExitCode_ConfEapTls_SetClientCertStoreIdentifier = 30,
    ExitCode_ConfEapTls_SetClientIdentity = 31,
    ExitCode_DuplicateState_DuplicateNetwork = 32,
    ExitCode_DuplicateState_PersistConfig = 33,
    ExitCode_EapTlsNetworkInformation_GetConnectedNetworkId = 34,
    ExitCode_EapTlsNetworkInformation_GetClientIdentity = 35,
    ExitCode_EapTlsNetworkInformation_GetClientCertStoreIdentifier = 36,
    ExitCode_EapTlsNetworkInformation_GetRootCACertStoreIdentifier = 37

} ExitCode;

// The MT3620 currently handles a maximum of 10 stored wifi networks.
static const unsigned int MAX_NUMBER_STORED_NETWORKS = 10;

// Network configuration: Configure the variables with the appropriate settings for your network
static const uint8_t sampleNetworkSsid[] = "WIFI_NETWORK_SSID";
static const WifiConfig_Security_Type sampleNetworkSecurityType = WifiConfig_Security_Unknown;

// Network configuration: Settings specific to an WPA2_PSK network
static const char *sampleNetworkPsk = "WIFI_NETWORK_PASSWORD";

// Network configuration: Settings specific to an EAP-TLS network
static const char *rootCACertStoreIdentifier = "SmplRootCACertId";
static const char *clientCertStoreIdentifier = "SmplClientCertId";
static const char *clientIdentity = "SmplClientId";

// By default, the configuration name for the new network will be
// set to 'SmplNetCfg'
static const char *sampleNetworkConfigName = "SmplNetCfg";

// By default, the new network will be duplicated
static const char *duplicatedNetworkConfigName = "SmplDupNetCfg";
static int duplicatedNetworkId = -1;
static const int authFailureDiagError = 5;

// Compute the SSID length based on the configured sampleNetworkSsid
static uint8_t sampleNetworkSsidLength =
    sizeof(sampleNetworkSsid) / sizeof(sampleNetworkSsid[0]) - 1;

// Array used to print the network security type as a string
static const char *securityTypeToString[] = {"Unknown", "Open", "WPA2/PSK", "EAP-TLS"};

// File descriptors - initialized to invalid value
static int changeNetworkConfigButtonGpioFd = -1;
static int showNetworkStatusButtonGpioFd = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// Button state variables
static GPIO_Value_Type changeNetworkConfigButtonState = GPIO_Value_High;
static GPIO_Value_Type showNetworkStatusButtonState = GPIO_Value_High;

static void TerminationHandler(int signalNumber);
static void StateStatusOutputHelper(const char *currentStateMessage, const char *nextStateMessage,
                                    bool statusIsSuccessful);
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
static ExitCode RetrieveNetworkDiagnostics(void);
static ExitCode OutputScannedWifiNetworks(void);
static ExitCode OutputEapTlsInformation(void);
static void ShowDeviceNetworkStatus(void);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void ButtonEventTimeHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

static int RetrieveNetworkIdByConfigName(const char *configName);
static ExitCode SetNetworkConfigNameForNetworkId(int networkId, const char *configName);

static ExitCode HelperWifiNetworkConfigureNetwork(void);
static ExitCode WifiNetworkConfigureEapTlsNetwork(void);
static ExitCode WifiNetworkConfigureWpaPskNetwork(void);
static ExitCode WifiNetworkConfigureOpenNetwork(void);

// Available states
static void WifiNetworkConfigureAndAddState(void);
static void WifiNetworkEnableState(void);
static void WifiNetworkDisableState(void);
static void WifiNetworkDuplicateState(void);
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
    if (!statusIsSuccessful) {
        Log_Debug("ERROR: Finished %s network with status: FAILED. The application will exit.\n",
                  currentStateMessage);
    }

    Log_Debug(
        "\nFinished %s network with status: SUCCESS. By pressing BUTTON_1 the network will be "
        "%s.\n",
        currentStateMessage, nextStateMessage);
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
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode WifiRetrieveStoredNetworks(ssize_t *numberOfNetworksStored,
                                           WifiConfig_StoredNetwork *storedNetworksArray)
{
    ssize_t temporaryNumberOfNetworks = WifiConfig_GetStoredNetworkCount();
    if (temporaryNumberOfNetworks == -1) {
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
    if (temporaryNumberOfNetworks == -1) {
        Log_Debug("ERROR: WifiConfig_GetStoredNetworks failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_RetrieveNetworks_GetStored;
    }
    for (size_t i = 0; i < temporaryNumberOfNetworks; ++i) {
        storedNetworksArray[i] = temporaryStoredNetworksArray[i];
    }

    *numberOfNetworksStored = temporaryNumberOfNetworks;

    return ExitCode_Success;
}

/// <summary>
///     Helper function used to retrieve the id of the new added network based on the
///     configuration name.
/// </summary>
/// <param name="configName">The configuration name used to retrieve the network id
/// </param>
/// <returns>
///     The network id on success; otherwise -1 and sets ExitCode value which indicates
///     the specific failure.
/// </returns>
static int RetrieveNetworkIdByConfigName(const char *configName)
{
    int sampleStoredNetworkId = WifiConfig_GetNetworkIdByConfigName(configName);
    if (sampleStoredNetworkId == -1) {
        Log_Debug("ERROR: WifiConfig_GetNetworkIdByConfigName failed: %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_RetrieveNetworkIdByConfigName_GetNetworkIdByConfigName;
    }

    return sampleStoredNetworkId;
}

/// <summary>
///     Helper function used to set the configuration name of for a network id.
/// </summary>
/// <param name="networkId">The network id to be associated with the configuration
/// name.</param>
/// <param name="configName">The configuration name to be associated with the network
/// id.</param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode SetNetworkConfigNameForNetworkId(int networkId, const char *configName)
{
    int result = WifiConfig_SetConfigName(networkId, configName);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetConfigName failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_SetNetworkConfigName_SetConfigName;
    }

    return ExitCode_Success;
}

/// <summary>
///     Helper function used to add and configure the SSID and network security type for
///     an EAP-TLS, Open, or WPA/PSK network.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode HelperWifiNetworkConfigureNetwork(void)
{
    int sampleStoredNetworkId = WifiConfig_AddNetwork();
    if (sampleStoredNetworkId == -1) {
        Log_Debug("ERROR: WifiConfig_AddNetwork failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_ConfAddState_AddNetwork;
    }

    int result = WifiConfig_SetSecurityType(sampleStoredNetworkId, sampleNetworkSecurityType);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetSecurityType failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_ConfAddState_SetSecType;
    }

    result = WifiConfig_SetSSID(sampleStoredNetworkId, sampleNetworkSsid, sampleNetworkSsidLength);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetSSID failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_ConfAddState_SetSsid;
    }

    exitCode = SetNetworkConfigNameForNetworkId(sampleStoredNetworkId, sampleNetworkConfigName);
    return exitCode;
}

/// <summary>
///     Configures and stores an EAP-TLS network based on the pre-existing certificates.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode WifiNetworkConfigureEapTlsNetwork(void)
{
    // If there is an existing EAP-TLS network with the same configuration,
    // and the certificates have to be changed (rollover), consider using
    // AddDuplicatedNetwork to copy the configuration of the existing network
    exitCode = HelperWifiNetworkConfigureNetwork();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return exitCode;
    }

    int result =
        WifiConfig_SetRootCACertStoreIdentifier(sampleStoredNetworkId, rootCACertStoreIdentifier);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetRootCACertStoreIdentifier failed: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_ConfEapTls_SetRootCACertStoreIdentifier;
    }

    result =
        WifiConfig_SetClientCertStoreIdentifier(sampleStoredNetworkId, clientCertStoreIdentifier);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetClientCertStoreIdentifier failed: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_ConfEapTls_SetClientCertStoreIdentifier;
    }

    result = WifiConfig_SetClientIdentity(sampleStoredNetworkId, clientIdentity);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetClientIdentity failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_ConfEapTls_SetClientIdentity;
    }

    return ExitCode_Success;
}

/// <summary>
///     Configures and stores a WPA/PSK network based on the SSID, network security type and the psk
///     configured.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode WifiNetworkConfigureWpaPskNetwork(void)
{
    exitCode = HelperWifiNetworkConfigureNetwork();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return exitCode;
    }

    // if the network security is Wpa2_Psk, set the Psk
    int result = WifiConfig_SetPSK(sampleStoredNetworkId, sampleNetworkPsk,
                                   strnlen(sampleNetworkPsk, WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE));
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetPSK failed: %s (%d).\n", strerror(errno), errno);
        return ExitCode_ConfAddState_SetPsk;
    }

    return ExitCode_Success;
}

/// <summary>
///     Configures and stores an Open network based on the SSID and the network security type.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode WifiNetworkConfigureOpenNetwork(void)
{
    return HelperWifiNetworkConfigureNetwork();
}

/// <summary>
///     Configures and stores a new network based on the SSID, network security type, and/or the
///     psk, and/or certificates provided and saves the configuration.
/// </summary>
static void WifiNetworkConfigureAndAddState(void)
{
    assert(sampleNetworkSsidLength < WIFICONFIG_SSID_MAX_LENGTH);

    if (sampleNetworkSecurityType == WifiConfig_Security_Unknown) {
        Log_Debug(
            "ERROR: sampleNetworkSecurityType should be set to WifiConfig_Security_Open,"
            " WifiConfig_Security_Wpa2_Psk, or WifiConfig_Security_Wpa2_EAP_TLS.\n");
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
    assert(numberOfNetworksStored < MAX_NUMBER_STORED_NETWORKS);

    switch (sampleNetworkSecurityType) {
    case WifiConfig_Security_Open:
        exitCode = WifiNetworkConfigureOpenNetwork();
        break;

    case WifiConfig_Security_Wpa2_Psk:
        exitCode = WifiNetworkConfigureWpaPskNetwork();
        break;

    case WifiConfig_Security_Wpa2_EAP_TLS:
        exitCode = WifiNetworkConfigureEapTlsNetwork();
        break;

    default:
        exitCode = ExitCode_ConfAddState_WrongSecType;
        Log_Debug(
            "ERROR: sampleNetworkSecurityType should be set to WifiConfig_Security_Open,"
            " WifiConfig_Security_Wpa2_Psk, or WifiConfig_Security_Wpa2_EAP_TLS.\n");
    }

    if (exitCode != ExitCode_Success) {
        Log_Debug("ERROR: Failed to configure a new network.\n");
        return;
    }

    // save the configuration
    int result = WifiConfig_PersistConfig();
    if (result == -1) {
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
    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return;
    }

    int result = WifiConfig_SetNetworkEnabled(sampleStoredNetworkId, true);
    if (result == -1) {
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
    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return;
    }

    int result = WifiConfig_SetNetworkEnabled(sampleStoredNetworkId, false);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_SetNetworkEnabled failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_DisableState_SetNetworkEnabled;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkDuplicateState;
    StateStatusOutputHelper("disabling the", "duplicated", true);
}

/// <summary>
///     Duplicates the existing network and saves the configuration.
/// </summary>
static void WifiNetworkDuplicateState(void)
{
    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return;
    }

    duplicatedNetworkId =
        WifiConfig_AddDuplicateNetwork(sampleStoredNetworkId, duplicatedNetworkConfigName);
    if (duplicatedNetworkId == -1) {
        Log_Debug("ERROR: WifiConfig_AddDuplicateNetwork failed: %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_DuplicateState_DuplicateNetwork;
        return;
    }

    int result = WifiConfig_PersistConfig();
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_PersistConfig failed: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_DuplicateState_PersistConfig;
        return;
    }

    // set the next state
    nextStateFunction = WifiNetworkDeleteState;
    StateStatusOutputHelper("duplicating the", "deleted", true);
}

/// <summary>
///     Deletes the configured and the duplicated networks and saves the configuration.
/// </summary>
static void WifiNetworkDeleteState(void)
{
    int sampleStoredNetworkId = RetrieveNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1) {
        return;
    }

    int result = WifiConfig_ForgetNetworkById(sampleStoredNetworkId);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_ForgetNetworkById (%d) failed: %s (%d).\n",
                  sampleStoredNetworkId, strerror(errno), errno);
        exitCode = ExitCode_DeleteState_ForgetNetworkById;
        return;
    }

    result = WifiConfig_ForgetNetworkById(duplicatedNetworkId);
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_ForgetNetworkById (%d) failed: %s (%d).\n",
                  duplicatedNetworkId, strerror(errno), errno);
        exitCode = ExitCode_DeleteState_ForgetNetworkById;
        return;
    }
    duplicatedNetworkId = -1;

    result = WifiConfig_PersistConfig();
    if (result == -1) {
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
    size_t j = 0;
    size_t i = 1;

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
        assert(deduplicatedScannedNetworks[i].security == WifiConfig_Security_Open ||
               deduplicatedScannedNetworks[i].security == WifiConfig_Security_Wpa2_Psk ||
               deduplicatedScannedNetworks[i].security == WifiConfig_Security_Wpa2_EAP_TLS ||
               deduplicatedScannedNetworks[i].security == WifiConfig_Security_Unknown);
        Log_Debug(" : %s : %d dB\n", securityTypeToString[deduplicatedScannedNetworks[i].security],
                  deduplicatedScannedNetworks[i].signalRssi);
    }
}

/// <summary>
///     Checks if the network is ready.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode CheckNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) == -1) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_IsNetworkingReady_Failed;
        return false;
    }

    if (isNetworkReady) {
        Log_Debug("INFO: Network is ready.\n");
    } else {
        Log_Debug("INFO: Network is not ready.\n");
    }

    return ExitCode_Success;
}

/// <summary>
///     Outputs specific information about the EAP-TLS network.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode OutputEapTlsInformation(void)
{
    int connectedNetworkId = WifiConfig_GetConnectedNetworkId();
    if (connectedNetworkId == -1 && errno != ENOTCONN) {
        Log_Debug("\nERROR: WifiConfig_GetConnectedNetworkId failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_EapTlsNetworkInformation_GetConnectedNetworkId;
    }

    if (connectedNetworkId == -1 && errno == ENOTCONN) {
        Log_Debug("WARNING: The device is not connected to a Wi-Fi network.\n");
        return ExitCode_Success;
    }

    WifiConfig_ClientIdentity outIdentity;
    int result = WifiConfig_GetClientIdentity(connectedNetworkId, &outIdentity);
    if (result == -1) {
        Log_Debug("\nERROR: WifiConfig_GetClientIdentity failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_EapTlsNetworkInformation_GetClientIdentity;
    }
    Log_Debug("INFO: Client identity is '%s'\n.", outIdentity.identity);

    CertStore_Identifier outIdentifier;
    result = WifiConfig_GetClientCertStoreIdentifier(connectedNetworkId, &outIdentifier);
    if (result == -1) {
        Log_Debug("\nERROR: WifiConfig_GetClientCertStoreIdentifier failed: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_EapTlsNetworkInformation_GetClientCertStoreIdentifier;
    }
    Log_Debug("INFO: Client certificate identifier '%s'\n.", outIdentifier.identifier);

    CertStore_Identifier rootCAOutIdentifier;
    result = WifiConfig_GetRootCACertStoreIdentifier(connectedNetworkId, &rootCAOutIdentifier);
    if (result == -1) {
        Log_Debug("\nERROR: WifiConfig_GetRootCACertStoreIdentifier failed: %s (%d).\n",
                  strerror(errno), errno);
        return ExitCode_EapTlsNetworkInformation_GetRootCACertStoreIdentifier;
    }
    Log_Debug("INFO: Root CA certificate identifier '%s'\n.", rootCAOutIdentifier.identifier);

    return ExitCode_Success;
}

/// <summary>
///     Checks if the current Wi-Fi network is enabled, connected and outputs its SSID,
///     RSSI signal and security type.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
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
        Log_Debug("INFO: The device is not connected to a Wi-Fi network.\n");
        return ExitCode_Success;
    }

    Log_Debug("INFO: The device is connected to: ");
    for (unsigned int i = 0; i < connectedNetwork.ssidLength; ++i) {
        Log_Debug("%c", isprint(connectedNetwork.ssid[i]) ? connectedNetwork.ssid[i] : '.');
    }
    assert(connectedNetwork.security == WifiConfig_Security_Open ||
           connectedNetwork.security == WifiConfig_Security_Wpa2_Psk ||
           connectedNetwork.security == WifiConfig_Security_Wpa2_EAP_TLS);
    Log_Debug(" : %s : %d dB\n", securityTypeToString[connectedNetwork.security],
              connectedNetwork.signalRssi);

    // Output information about the client and the certificates
    if (connectedNetwork.security == WifiConfig_Security_Wpa2_EAP_TLS) {
        return OutputEapTlsInformation();
    }

    return ExitCode_Success;
}

/// <summary>
///    Outputs the stored Wi-Fi networks.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
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
        assert(storedNetworksArray[i].security <= WifiConfig_Security_Wpa2_EAP_TLS);
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
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode OutputScannedWifiNetworks(void)
{
    // Check the available Wi-Fi networks
    ssize_t numberOfNetworks = WifiConfig_TriggerScanAndGetScannedNetworkCount();
    if (numberOfNetworks == -1) {
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
    if (numberOfScannedNetworks == -1) {
        Log_Debug("ERROR: WifiConfig_GetScannedNetworks failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_OutputScanned_GetScanned;
    }

    SortAndDeduplicateAvailableNetworks(scannedNetworksArray, (size_t)numberOfScannedNetworks);

    return ExitCode_Success;
}

/// <summary>
///     Retrieves the network diagnostics.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode RetrieveNetworkDiagnostics(void)
{
    int sampleStoredNetworkId = WifiConfig_GetNetworkIdByConfigName(sampleNetworkConfigName);
    if (sampleStoredNetworkId == -1 && errno != ENODEV) {
        Log_Debug("ERROR: WifiConfig_GetNetworkIdByConfigName failed: %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_RetrieveNetworkDiagnostics_GetNetworkIdByConfigName;
    }

    if (sampleStoredNetworkId == -1 && errno == ENODEV) {
        Log_Debug(
            "WARNING: Can't retrieve the network diagnostics. Add and configure a network before "
            "using this functionality.\n");
        return ExitCode_Success;
    }

    WifiConfig_NetworkDiagnostics networkDiagnostics;
    int result = WifiConfig_GetNetworkDiagnostics(sampleStoredNetworkId, &networkDiagnostics);
    if (result == -1 && errno != ENODEV) {
        Log_Debug("ERROR: WifiConfig_GetNetworkDiagnostics failed: %s (%d).\n", strerror(errno),
                  errno);
        return ExitCode_RetrieveNetworkDiag_GetNetworkDiagnostics;
    }

    if (result == -1 && errno == ENODEV) {
        Log_Debug("INFO: Couldn't find any diagnostic information for network ID %d.\n",
                  sampleStoredNetworkId);
        return ExitCode_Success;
    }

    Log_Debug("INFO: The network is '%s'.\n",
              networkDiagnostics.isEnabled ? "enabled" : "disabled");
    Log_Debug("INFO: The network is '%s'.\n",
              networkDiagnostics.isConnected ? "connected" : "disconnected");
    Log_Debug(
        "INFO: The last reason to fail to connect to the network was: %d. Check 'wificonfig.h' "
        "to identify the reason of the error.\n",
        networkDiagnostics.error);
    Log_Debug("INFO: Last network connection failure happened at %s.\n",
              ctime(&networkDiagnostics.timestamp));

    // Check if the network error was due to an authentication failure
    if (networkDiagnostics.error != authFailureDiagError) {
        return ExitCode_Success;
    }

    // The following information is meaningful only when 'error' indicates that the
    // authentication has failed
    Log_Debug(
        "INFO: Certificate error: %d. Check 'wificonfig.h' to identify the reason of the "
        "error.\n",
        networkDiagnostics.certError);
    Log_Debug("INFO: The certificate's subject is '%s'.\n", networkDiagnostics.certSubject.name);

    if (networkDiagnostics.certDepth >= 0) {
        Log_Debug("INFO: The certificate's depth in the certification chain is %d.\n",
                  networkDiagnostics.certDepth);
    }

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
        localExitCode = RetrieveNetworkDiagnostics();
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
/// Button timer event:  Check the status of the buttons.
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
    changeNetworkConfigButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (changeNetworkConfigButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_SampleButton;
    }

    // By pressing BUTTON_1 the WifiNetworkConfigureAndAddState will be called
    nextStateFunction = WifiNetworkConfigureAndAddState;

    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    showNetworkStatusButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (showNetworkStatusButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
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
        "Each press of BUTTON_1 will advance through a cycle that adds, enables, disables, "
        "duplicates, and deletes a Wi-Fi example network.\n");
    Log_Debug(
        "Press BUTTON_2 to check if the device is connected to a Wi-Fi network, to retrieve the "
        "network diagnostics, to trigger a Wi-Fi network scan, and to print a deduplicated list of "
        "available Wi-Fi networks.\n");

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