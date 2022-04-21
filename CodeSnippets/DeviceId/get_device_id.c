/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
// This code snippet demonstrates how to read the Device ID

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <applibs/log.h>
#include <applibs/application.h>
#include <tlsutils/deviceauth.h>
#include <wolfssl/ssl.h>

// Calling the GetDeviceID function
// #define DEVICE_ID_BUFFER_SIZE 130
// char device_id[DEVICE_ID_BUFFER_SIZE];	// Device ID is 128 bytes
// 
// GetDeviceID Function Definition
// int GetDeviceID(char* deviceId, size_t deviceIdLength);
//
// if (GetDeviceID((char*)&device_id, DEVICE_ID_BUFFER_SIZE) == 0)
//     Log_Debug("DeviceID: %s\n", device_id);


int GetDeviceID(char* deviceId, size_t deviceIdLength)
{
    int result = -1;
    bool wolfSslInitialized = false;
    WOLFSSL_X509* deviceCert = NULL;

    bool isDeviceAuthReady = false;
    if (Application_IsDeviceAuthReady(&isDeviceAuthReady) < 0) {
        Log_Debug("ERROR: Device authentication could not be checked: %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    if (!isDeviceAuthReady) {
        Log_Debug("ERROR: Device has not authenticated: %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_Init(): %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }
    wolfSslInitialized = true;

    deviceCert = wolfSSL_X509_load_certificate_file(DeviceAuth_GetCertificatePath(), WOLFSSL_FILETYPE_PEM);
    if (deviceCert == NULL)
    {
        Log_Debug("wolfSSL_X509_load_certificate_file error %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    WOLFSSL_X509_NAME* subjectName = wolfSSL_X509_get_subject_name(deviceCert);
    if (subjectName == NULL) {
        Log_Debug("ERROR: invalid data: %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    char localDeviceId[134] = { 0 };

    if (wolfSSL_X509_NAME_oneline(subjectName, (char*)&localDeviceId, sizeof(localDeviceId)) < 0)
    {
        Log_Debug("ERROR: Failed to get device id: %d (%s)\n", errno, strerror(errno));
        goto cleanup;
    }

    snprintf(deviceId, deviceIdLength, "%s", localDeviceId + 4);

    result = 0;

cleanup:
    if (deviceCert != NULL) {
        wolfSSL_X509_free(deviceCert);
    }

    if (wolfSslInitialized) {
        wolfSSL_Cleanup();
    }

    return result;
}

