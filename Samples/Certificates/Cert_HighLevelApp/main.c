/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the certificate
// store APIs. Each press of SAMPLE_BUTTON_1 will advance through a cycle that installs,
// moves certificates, reloads the Wi-Fi network (step required for an EAP-TLS network)
// and deletes the certificates. SAMPLE_BUTTON_2 displays the available space on the divice,
// lists the installed certificates, and displays specific information about each certificate.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (displays messages in the Device Output window during debugging)
// - certstore (functions and types that interact with certificates)
// - wificonfig (functions and types that interact with networking)

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/certstore.h>
#include <applibs/wificonfig.h>

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

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_IsButtonPressed_GetValue = 2,

    ExitCode_ButtonTimerHandler_Consume = 3,

    ExitCode_Validate_RootCACertificate = 4,
    ExitCode_Validate_ClientCertificate = 5,

    ExitCode_CheckAvailableSpace_GetAvailableSpace = 6,
    ExitCode_CheckAvailableSpace_NotEnoughSpace = 7,

    ExitCode_InstallState_InstallClientCertificate = 8,
    ExitCode_InstallState_InstallRootCACertificate = 9,

    ExitCode_InstallNewState_InstallSecondRootCACertificate = 10,

    ExitCode_DisplayCertInformation_GetAvailableSpace = 11,
    ExitCode_DisplayCertInformation_GetCertificateCount = 12,
    ExitCode_DisplayCertInformation_GetCertificateIdentifierAt = 13,
    ExitCode_DisplayCertInformation_GetCertificateSubjectName = 14,
    ExitCode_DisplayCertInformation_GetCertificateIssuerName = 15,
    ExitCode_DisplayCertInformation_GetCertificateNotBefore = 16,
    ExitCode_DisplayCertInformation_GetCertificateNotAfter = 17,

    ExitCode_RootCACertMoveState_MoveCertificate = 18,

    ExitCode_WifiReloadConfigState_ReloadConfig = 19,

    ExitCode_CertDeleteState_DeleteCertificate = 20,

    ExitCode_Init_SampleButton = 21,
    ExitCode_Init_EventLoop = 22,
    ExitCode_Init_ButtonTimer = 23,

    ExitCode_Main_EventLoopFail = 24
} ExitCode;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;
static void TerminationHandler(int signalNumber);

// Certificate identifiers
static const char *rootCACertIdentifier = "SmplRootCACertId";
static const char *newRootCACertIdentifier = "NewRootCACertId";
static const char *clientCertIdentifier = "SmplClientCertId";

// Configure the variable with the content of the root CA certificate
static const char *rootCACertContent = "root_ca_cert_content";

// Configure the variable with the content of the new root CA certificate
static const char *newRootCACertContent = "new_root_ca_cert_content";

// Client certificate content
static const char *clientCertContent = "client_cert_content";

// Configure the variable with the content of the client private key
static const char *clientPrivateKeyContent = "client_private_key_content";

// Configure the variable with the password of the client private key
static const char *clientPrivateKeyPassword = "client_private_key_password";

// File descriptors - initialized to invalid value
static int advanceCertSampleStateButtonGpioFd = -1;
static int showCertStatusButtonGpioFd = -1;

// Button state variables
static GPIO_Value_Type advanceCertSampleStateButtonState = GPIO_Value_High;
static GPIO_Value_Type showCertStatusButtonState = GPIO_Value_High;

static bool IsButtonPressed(int fd, GPIO_Value_Type *buttonState);
static void ButtonEventTimeHandler(EventLoopTimer *timer);

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

// Available states
static void CertInstallState(void);
static void InstallNewRootCACertificateState(void);
static void RootCACertMoveState(void);
static void WifiReloadConfigState(void);
static void CertDeleteState(void);

// Helper functions
static bool CheckDeviceSpaceForInstallation(size_t certificateSize);
static void DisplayCertInformation(void);

// Pointer to the next state
typedef void (*NextStateFunctionPtr)(void);

// Each Button_1 press will advance the state
static NextStateFunctionPtr nextStateFunction = NULL;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="buttonState">Old state of the button (pressed or released)
/// which will be updated to the current button state.
/// </param>
/// <returns>True if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *buttonState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *buttonState) && (newState == GPIO_Value_Low);
        *buttonState = newState;
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
        IsButtonPressed(advanceCertSampleStateButtonGpioFd, &advanceCertSampleStateButtonState);
    if (isButtonPressed) {
        nextStateFunction();
    }

    // Check if BUTTON_2 was pressed
    isButtonPressed = IsButtonPressed(showCertStatusButtonGpioFd, &showCertStatusButtonState);
    if (isButtonPressed) {
        DisplayCertInformation();
    }
}

/// <summary>
///     Checks whether there is enough available space to install a certificate.
/// </summary>
/// <param name="certificateSize">Certificate size.</param>
/// <returns>True if there is enough available space and the API call has succeeded,
/// false otherwise</returns>
static bool CheckDeviceSpaceForInstallation(size_t certificateSize)
{
    ssize_t availableSpace = CertStore_GetAvailableSpace();
    if (availableSpace == -1) {
        Log_Debug("ERROR: CertStore_GetAvailableSpace has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_CheckAvailableSpace_GetAvailableSpace;
        return false;
    } else if (((size_t)availableSpace) < certificateSize) {
        Log_Debug("ERROR: Available space (%zu) is less than the required space: (%zu).\n",
                  availableSpace, certificateSize);
        exitCode = ExitCode_CheckAvailableSpace_NotEnoughSpace;
        return false;
    }

    return true;
}

/// <summary>
///     Displays information about the installed certificates.
/// </summary>
static void DisplayCertInformation(void)
{
    ssize_t availableSpace = CertStore_GetAvailableSpace();
    if (availableSpace == -1) {
        Log_Debug("ERROR: CertStore_GetAvailableSpace has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_DisplayCertInformation_GetAvailableSpace;
        return;
    }
    Log_Debug("INFO: Available space in device certificate store: %zu B.\n", availableSpace);

    ssize_t certCount = CertStore_GetCertificateCount();
    if (certCount == -1) {
        Log_Debug("ERROR: CertStore_GetCertificateCount has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_DisplayCertInformation_GetCertificateCount;
        return;
    }

    if (certCount == 0) {
        Log_Debug("INFO: No certificates installed on this device.\n");
        return;
    }
    Log_Debug("INFO: There are %d certificate(s) installed on this device.\n", certCount);

    for (size_t i = 0; i < certCount; ++i) {
        CertStore_Identifier certIdentifier;
        int result = CertStore_GetCertificateIdentifierAt(i, &certIdentifier);
        if (result == -1) {
            Log_Debug("ERROR: CertStore_GetCertificateIdentifierAt has failed: errno = %s (%d).\n",
                      strerror(errno), errno);
            exitCode = ExitCode_DisplayCertInformation_GetCertificateIdentifierAt;
            return;
        }
        Log_Debug("INFO: Certificate %d has identifier: '%s'.\n", i, certIdentifier.identifier);

        CertStore_SubjectName outSubjectName;
        result = CertStore_GetCertificateSubjectName(certIdentifier.identifier, &outSubjectName);
        if (result == -1) {
            Log_Debug("\tERROR: CertStore_GetCertificateSubjectName has failed: errno = %s (%d).\n",
                      strerror(errno), errno);
            exitCode = ExitCode_DisplayCertInformation_GetCertificateSubjectName;
            return;
        }
        Log_Debug("\tINFO: Certificate subject name: '%s'.\n", outSubjectName.name);

        CertStore_IssuerName outIssuerName;
        result = CertStore_GetCertificateIssuerName(certIdentifier.identifier, &outIssuerName);
        if (result == -1) {
            Log_Debug("\tERROR: CertStore_GetCertificateIssuerName has failed: errno = %s (%d).\n",
                      strerror(errno), errno);
            exitCode = ExitCode_DisplayCertInformation_GetCertificateIssuerName;
            return;
        }
        Log_Debug("\tINFO: Certificate issuer name: '%s'.\n", outIssuerName.name);

        struct tm outNotBefore;
        result = CertStore_GetCertificateNotBefore(certIdentifier.identifier, &outNotBefore);
        if (result == -1) {
            Log_Debug("\tERROR: CertStore_GetCertificateNotBefore has failed: errno = %s (%d).\n",
                      strerror(errno), errno);
            exitCode = ExitCode_DisplayCertInformation_GetCertificateNotBefore;
            return;
        }
        char timeBuf[64];
        if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T", &outNotBefore) != 0) {
            Log_Debug("\tINFO: Certificate not before validity date: %s\n", timeBuf);
        }

        struct tm outNotAfter;
        result = CertStore_GetCertificateNotAfter(certIdentifier.identifier, &outNotAfter);
        if (result == -1) {
            Log_Debug("ERROR: CertStore_GetCertificateNotAfter has failed: errno = %s (%d).\n",
                      strerror(errno), errno);
            exitCode = ExitCode_DisplayCertInformation_GetCertificateNotAfter;
            return;
        }
        if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T", &outNotAfter) != 0) {
            Log_Debug("\tINFO: Certificate not after validity date: %s\n", timeBuf);
        }
    }
}

/// <summary>
///     Installs the certificates.
/// </summary>
static void CertInstallState(void)
{
    size_t rootCACertContentSize = 0;
    if (rootCACertContent != NULL) {
        rootCACertContentSize = strlen(rootCACertContent);
    }
    if (!CheckDeviceSpaceForInstallation(rootCACertContentSize)) {
        Log_Debug(
            "ERROR: Failed to install the root CA and client certificates because there isn't "
            "enough space on the device.\n");
        exitCode = ExitCode_InstallState_InstallRootCACertificate;
        return;
    }
    int result = CertStore_InstallRootCACertificate(rootCACertIdentifier, rootCACertContent,
                                                    rootCACertContentSize);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_InstallRootCACertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_InstallState_InstallRootCACertificate;
        return;
    }

    size_t clientCertContentSize = 0;
    if (clientCertContent != NULL) {
        clientCertContentSize = strlen(clientCertContent);
    }
    if (!CheckDeviceSpaceForInstallation(clientCertContentSize)) {
        Log_Debug(
            "ERROR: Failed to install the client certificate because there isn't enough space on "
            "the device.\n");
        exitCode = ExitCode_InstallState_InstallClientCertificate;
        return;
    }

    size_t privateKeyContentSize = 0;
    if (clientPrivateKeyContent != NULL) {
        privateKeyContentSize = strlen(clientPrivateKeyContent);
    }
    result = CertStore_InstallClientCertificate(clientCertIdentifier, clientCertContent,
                                                clientCertContentSize, clientPrivateKeyContent,
                                                privateKeyContentSize, clientPrivateKeyPassword);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_InstallClientCertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_InstallState_InstallClientCertificate;
        return;
    }

    // set the next state
    nextStateFunction = InstallNewRootCACertificateState;
    Log_Debug(
        "Finished installing the root CA and the client certificates with status: SUCCESS. By "
        "pressing BUTTON_1 the new root CA certificate will be installed.\n");
}

/// <summary>
///    Installs an additional root CA certificate.
/// </summary>
static void InstallNewRootCACertificateState(void)
{
    size_t newRootCACertContentSize = 0;
    if (newRootCACertContent != NULL) {
        newRootCACertContentSize = strlen(newRootCACertContent);
    }
    if (!CheckDeviceSpaceForInstallation(newRootCACertContentSize)) {
        Log_Debug(
            "ERROR: Failed to install the root CA and client certificates because there isn't "
            "enough space on the device.\n");
        exitCode = ExitCode_InstallNewState_InstallSecondRootCACertificate;
        return;
    }
    int result = CertStore_InstallRootCACertificate(newRootCACertIdentifier, newRootCACertContent,
                                                    newRootCACertContentSize);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_InstallClientCertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_InstallNewState_InstallSecondRootCACertificate;
        return;
    }

    // set the next state
    nextStateFunction = RootCACertMoveState;
    Log_Debug(
        "Finished installing the new root CA certificate with status: SUCCESS. By pressing "
        "BUTTON_1 the root CA certificate will be replaced by the new root CA certificate.\n");
}

/// <summary>
///     Replace the certificate identified by rootCACertIdentifier with the certificate identified
///     by newRootCACertIdentifier. The certificate data previously identified by
///     rootCACertIdentifier will be deleted, and the the identifier newRootCACertIdentifier will no
///     longer be valid.
/// </summary>
static void RootCACertMoveState(void)
{
    int result = CertStore_MoveCertificate(newRootCACertIdentifier, rootCACertIdentifier);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_MoveCertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_RootCACertMoveState_MoveCertificate;
        return;
    }

    // set the next state
    nextStateFunction = WifiReloadConfigState;
    Log_Debug(
        "Finished replacing the root CA certificate with the new root CA certificate with status: "
        "SUCCESS. By pressing BUTTON_1 the device Wi-Fi configuration will be reloaded.\n");
}

/// <summary>
///    Reload the device Wi-Fi configuration following changes to the certificate store.
///    It is necessary to reload the Wi-Fi config after making any change to the certificate store,
///    in order to make the changes available for configuring an EAP-TLS network.
/// </summary>
static void WifiReloadConfigState(void)
{
    int result = WifiConfig_ReloadConfig();
    if (result == -1) {
        Log_Debug("ERROR: WifiConfig_ReloadConfig has failed: errno = %s (%d).\n", strerror(errno),
                  errno);
        exitCode = ExitCode_WifiReloadConfigState_ReloadConfig;
        return;
    }

    // set the next state
    nextStateFunction = CertDeleteState;
    Log_Debug(
        "Finished reloading the Wi-Fi configuration with status: SUCCESS. By pressing BUTTON_1 the "
        "new root CA and client certificates will be deleted.\n");
}

/// <summary>
///    Deletes the installed certificates.
/// </summary>
static void CertDeleteState(void)
{
    int result = CertStore_DeleteCertificate(rootCACertIdentifier);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_DeleteCertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_CertDeleteState_DeleteCertificate;
        return;
    }
    Log_Debug("INFO: Erased certificate with identifier: %s.\n", rootCACertIdentifier);

    result = CertStore_DeleteCertificate(clientCertIdentifier);
    if (result == -1) {
        Log_Debug("ERROR: CertStore_DeleteCertificate has failed: errno = %s (%d).\n",
                  strerror(errno), errno);
        exitCode = ExitCode_CertDeleteState_DeleteCertificate;
        return;
    }
    Log_Debug("INFO: Erased certificate with identifier: %s.\n", clientCertIdentifier);

    // set the next state
    nextStateFunction = CertInstallState;
    Log_Debug(
        "Finished deleting the new root CA and client certificates with status: SUCCESS. By "
        "pressing BUTTON_1 the root CA, new root CA, and client certificates will be installed.\n");
}

/// <summary>
///     Validates if the values of the certificate identifier names were changed.
/// </summary>
/// <returns>ExitCode_Success if the certificate names were changed; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode ValidateUserConfiguration(void)
{
    exitCode = ExitCode_Success;

    if (rootCACertContent != NULL && strcmp(rootCACertContent, "root_ca_cert_content") == 0) {
        Log_Debug(
            "ERROR: Please ensure that you have modified the root CA certificate content before "
            "running this sample.\n");
        exitCode = ExitCode_Validate_RootCACertificate;
    }

    if (newRootCACertContent != NULL &&
        strcmp(newRootCACertContent, "new_root_ca_cert_content") == 0) {
        Log_Debug(
            "ERROR: Please ensure that you have modified the new root CA certificate content "
            "before running this sample.\n");
        exitCode = ExitCode_Validate_RootCACertificate;
    }

    if (clientCertContent != NULL && strcmp(clientCertContent, "client_cert_content") == 0) {
        Log_Debug(
            "ERROR: Please ensure that you have modified the client certificate content before "
            "running this sample.\n");
        exitCode = ExitCode_Validate_ClientCertificate;
    }

    if (clientPrivateKeyContent != NULL &&
        strcmp(clientPrivateKeyContent, "client_private_key_content") == 0) {
        Log_Debug(
            "ERROR: Please ensure that you have modified the client private key content before "
            "running this sample.\n");
        exitCode = ExitCode_Validate_ClientCertificate;
    }

    if (clientPrivateKeyPassword != NULL &&
        strcmp(clientPrivateKeyPassword, "client_private_key_password") == 0) {
        Log_Debug(
            "ERROR: Please ensure that you have modified the client private key password before "
            "running this sample.\n");
        exitCode = ExitCode_Validate_ClientCertificate;
    }

    return exitCode;
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

    exitCode = ValidateUserConfiguration();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    advanceCertSampleStateButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (advanceCertSampleStateButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_SampleButton;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    showCertStatusButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (showCertStatusButtonGpioFd < 0) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_SampleButton;
    }

    // By pressing BUTTON_1 the CertConfigureState will be called
    nextStateFunction = CertInstallState;

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
    CloseFdAndPrintError(advanceCertSampleStateButtonGpioFd, "Button2Gpio");
}

int main(void)
{
    Log_Debug("Cert application starting.\n");
    Log_Debug(
        "Each press of BUTTON_1 will advance through a cycle that installs, moves certificates, "
        "reloads the Wi-Fi network and deletes the certificates.\n");
    Log_Debug(
        "BUTTON_2 displays the available space on the device, lists the installed certificates, "
        "and displays specific information about each certificate.\n");

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