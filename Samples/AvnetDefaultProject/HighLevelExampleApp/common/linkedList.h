#pragma once

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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <applibs/log.h>
#include "../common/exitcodes.h"
#include "signal.h"
#include "build_options.h"

#if defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    

extern volatile sig_atomic_t exitCode;

// Define the linked list node structure
typedef struct telemetryNode {
	struct telemetryNode* next;
	struct telemetryNode* prev;
	char telemetryJson[]; // Dynamic array to hold the telemetry message text
} telemetryNode_t;

 telemetryNode_t* head; // global variable - pointer to head node.

void InitLinkedList(void);
telemetryNode_t* GetNewNode(const char* telemetryJson, size_t stringLen);
telemetryNode_t* InsertAtHead(char* x, int stringLen);
telemetryNode_t* InsertAtTail(char* x, int stringLen);
bool DeleteNode(telemetryNode_t* nodeToRemove);
void DeleteEntireList(void);
void Print(void);
void ReversePrint(void);
#endif // defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    
