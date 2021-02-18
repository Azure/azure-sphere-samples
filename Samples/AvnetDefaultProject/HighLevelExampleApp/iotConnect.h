/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef IOT_CONNECT_H
#define IOT_CONNECT_H

#include <signal.h>
#include <stdio.h>
#include <string.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>

#include "eventloop_timer_utilities.h"
#include "parson.h" // Used to parse Device Twin messages.

// Azure IoT SDK
#include <azure_sphere_provisioning.h>

#include "exit_codes.h"

// Provide access to global variables from main.c
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;
extern EventLoop *eventLoop;
extern volatile sig_atomic_t exitCode;
extern bool IoTCConnected;

// Provide access to core functions in main.c
extern void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context);
extern bool IsConnectionReadyToSendTelemetry(void);
extern void SendTelemetry(const char *, bool);

#define IOT_CONNECT_TELEMETRY_BUFFER_SIZE 256
#define GUID_LEN 36
#define SID_LEN 64
#define IOTC_HELLO_TELEMETRY_SIZE 128
#define IOTC_TELEMETRY_OVERHEAD 256

// Define tthe IoTConnect functios that get called from main.c
// void SendIoTConnectTelemetry(const char *jsonMessage);
bool FormatTelemetryForIoTConnect(const char *, char *, size_t);
ExitCode IoTConnectInit(void);
void IoTConnectConnectedToIoTHub(void);

#endif