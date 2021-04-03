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

#ifndef C_M4_SUPPORT_H
#define C_M4_SUPPORT_H

#include <sys/socket.h>
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
#include "exit_codes.h"
#include "iotConnect.h"
#include <applibs/log.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/application.h>

// Define the different messages IDs we can send to real time applications
typedef enum
{
	IC_UNKNOWN, 
	IC_HEARTBEAT,
	IC_READ_SENSOR,    
	IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_SET_SAMPLE_RATE
} INTER_CORE_CMD;

// Define the different real time interface versions.  This allows us to 
// expand this implementation in the future without having to touch
// all existing/legacy applications
typedef enum
{
	V0 = 0 // The initial interface version
	
} INTER_CORE_IMPLEMENTATION_VERSION;


typedef struct
{
	uint8_t cmd;
	uint32_t sensorSampleRate;
	void* applicationSpecificDataStruture;
} IC_COMMAND_RESPONSE_BLOCK;

// Variables and routines that the M4 interface needs to access
extern EventLoop *eventLoop;
extern volatile sig_atomic_t exitCode;
extern void SendTelemetry(const char*, bool);
extern void TwinReportState(const char*);

// Variables other files need to access
extern int realTimeAutoTelemetryInterval;

////////////////////////////////////////////////////////////
//
// M4 functions that can be called from main.c
//
////////////////////////////////////////////////////////////

sig_atomic_t InitM4Interfaces(void);
void CleanupM4Resources(void);
void RequestRawData(void);
void RequestRealTimeTelemetry(void);

//////////////////////////////////////////////////////////////////
// Define function signatures for M4 processing
//////////////////////////////////////////////////////////////////

// The m4InitFunction (required) will be called at powerup from the m4Init() routine
// This routine will setup the socket
typedef sig_atomic_t (*m4InitFunction)(void*);

// The m4Handler is called when the M4 socket receives a message
typedef void (*m4HandlerFunction)(EventLoop*, int, EventLoop_IoEvents, void*);

// The m4RawData handler is called when a raw data response message is recieved from the real time application
typedef void (*m4RawDataFunction)(void*);

// The m4Cleanup handler is called at system exit to cleanup/release any system resources
typedef void (*m4Cleanup)(void*);

// The m4RequestTelemety handler is called when the High level application wants to send telemetry from the realtime application
typedef void (*m4RequestTelemetry)(void*);

// Define the real time interface structure.  We'll create an array of these to process each real time application
typedef struct {
	char* m4Name;
    char* m4RtComponentID;
	m4InitFunction m4InitHandler;
	m4HandlerFunction m4Handler;
	m4RawDataFunction m4rawDataHandler;
	m4Cleanup m4CleanupHandler;
	m4RequestTelemetry m4TelemetryHandler;
	int m4Fd;
	uint8_t m4InterfaceVersion;
} m4_support_t;

/////////////////////////////////////////////////////////////////////////////////////
// Define generic handlers that will cover the common cases for real time applications
/////////////////////////////////////////////////////////////////////////////////////
sig_atomic_t genericM4Init(void*);
void genericM4Handler(EventLoop*, int , EventLoop_IoEvents, void*);
void genericM4Cleanup(void*);
void genericM4RequestTelemetry(void*);

void sendRealTimeTelemetryInterval(INTER_CORE_CMD, uint32_t);
int findArrayIndexByFd(int);

/////////////////////////////////////////////////////////////////////////////////////
// Define real time application specific functions
/////////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_GROVE_GPS_RT_APP
void groveGPSRawDataHandler(void*);
#endif 
#ifdef ENABLE_ALS_PT19_RT_APP
void alsPt19RawDataHandler(void*);
#endif
#ifdef ENABLE_GENERIC_RT_APP
void referenceRawDataHandler(void*);
#endif
#endif // C_M4_SUPPORT_H