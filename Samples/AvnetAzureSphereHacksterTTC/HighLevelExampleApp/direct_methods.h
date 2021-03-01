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

#ifndef C_DIRECT_METHODS_H
#define C_DIRECT_METHODS_H

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "applibs_versions.h"
#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"
#include "parson.h"
#include "signal.h"
#include "build_options.h"
#include <applibs/log.h>
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <applibs/powermanagement.h>

extern EventLoop *eventLoop;
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;
extern volatile sig_atomic_t exitCode;
sig_atomic_t InitDirectMethods(void);
void CleanupDirectMethods(void);

extern int DeviceMethodCallback(const char *methodName, const unsigned char *payload, size_t payloadSize, unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback);

//////////////////////////////////////////////////////////////////
// Define the signatures for the required direct method processing
//////////////////////////////////////////////////////////////////

// The dmInitFunction if defined will be called at powerup from the dmInit() routine
typedef sig_atomic_t (*dmInitFunction)(void*);

// The dmHandler takes the payload to process and returns a an HTTP result and a pointer to a response message on the heap
typedef int (*dmHandler)(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responsePayload);

// The dmCleanup handler is called at system exit to cleanup/release any system resources
typedef void (*dmCleanup)(void);

// Define the direct method structure.  We'll create an array of these to process
typedef struct {
	char* dmName;
	dmInitFunction dmInit;
	dmHandler dmHandler;
	dmCleanup dmCleanup;
	bool dmPayloadRequired;
} direct_method_t;

//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for test directMethod
//
//////////////////////////////////////////////////////////////////////////////////////
sig_atomic_t dmTestInitFunction(void* thisDmEntry);
int dmTestHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responsePayload);
void dmTestCleanupFunction(void);

//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions/variables for setTelemetryTxTime directMethod
//
//////////////////////////////////////////////////////////////////////////////////////
extern EventLoopTimer *sensorPollTimer;
int dmSetTelemetryTxTimeHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responsePayload);

//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for reboot directMethod
//
//////////////////////////////////////////////////////////////////////////////////////
extern EventLoopTimer *rebootDeviceTimer;
sig_atomic_t dmRebootInitFunction(void* thisDmEntry);
int dmRebootHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char** responsePayload);
void dmRebootCleanupFunction(void);


#endif 