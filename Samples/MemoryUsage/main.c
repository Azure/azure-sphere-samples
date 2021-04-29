/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates the Applications APIs by
// allocating memory and opening sockets. The application starts by allocating a
// buffer of 4,000 integers. It continues to allocate nodes and append them to a
// linked list. After it allocates MAX nodes, it frees the initial allocated buffer
// of memory and deletes the linked list. The app continues to append new nodes to
// the linked list and to erase them.
//
// It uses the API for the following Azure Sphere application libraries:
// - applications (acquire information about the memory usage)
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for IO events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/eventloop.h>
#include <applibs/applications.h>

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_AllocateMemoryTimer_Consume = 2,

    ExitCode_Init_EventLoop = 3,
    ExitCode_Init_AllocateMemoryTimer = 4,
    ExitCode_Init_AllocateMemoryBuffer = 5,

    ExitCode_AppendNode_CreateNode = 6,
    ExitCode_AppendNode_AllocateUserData = 7,
    ExitCode_AppendNode_CreateSocket = 8,

    ExitCode_Main_EventLoopFail = 9
} ExitCode;

// File descriptors - initialized to invalid value
static EventLoop *eventLoop = NULL;
static EventLoopTimer *allocateMemoryTimer = NULL;

// Data size in bytes
static const size_t USER_MEMORY_BUFFER_NO_ELEM = 4000;
static int *userMemoryBuffer = NULL;

typedef struct Node {
    int *userData;
    int socketFd;
    struct Node *next;
} Node;
static struct Node *linkedListHead = NULL;
static void AppendNode(Node **headNode);
static void DeleteList(Node **headNode);

static const size_t NODE_USER_DATA_NO_ELEM = 125;
static const int MAX_NUMBER_NODES = 8;
static int listSize = 0;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void PrintMemoryUsage(void);
static void TerminationHandler(int signalNumber);
static void AllocateMemoryTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Appends a node to the linked list.
/// </summary>
static void AppendNode(struct Node **headNode)
{
    Log_Debug("\nAppending a node in the linked list.\n");
    struct Node *lastNode = *headNode;
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        Log_Debug("ERROR: Could not create a new node %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_AppendNode_CreateNode;
        return;
    }

    // Allocate and initialize the memory
    newNode->userData = (int *)calloc(NODE_USER_DATA_NO_ELEM, sizeof(int));
    if (newNode->userData == NULL) {
        Log_Debug("ERROR: Could not initialize user data %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_AppendNode_AllocateUserData;
        free(newNode);
        return;
    }

    newNode->socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (newNode->socketFd == -1) {
        Log_Debug("ERROR: Could not create socket %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_AppendNode_CreateSocket;
        free(newNode->userData);
        free(newNode);
        return;
    }

    newNode->next = NULL;
    listSize++;

    if (*headNode == NULL) {
        *headNode = newNode;
        return;
    }

    while (lastNode->next != NULL) {
        lastNode = lastNode->next;
    }

    lastNode->next = newNode;
}

/// <summary>
///     Erases the linked list.
/// </summary>
static void DeleteList(struct Node **headNode)
{
    Log_Debug("Delete the linked list.\n");
    struct Node *currentNode = *headNode;

    while (currentNode != NULL) {
        struct Node *nextNode = currentNode->next;
        free(currentNode->userData);
        close(currentNode->socketFd);
        free(currentNode);
        currentNode = nextNode;
    }

    *headNode = NULL;
    listSize = 0;
}

/// <summary>
///     Prints the memory usage.
/// </summary>
static void PrintMemoryUsage(void)
{
    size_t userMemoryUsage = Applications_GetUserModeMemoryUsageInKB();
    Log_Debug("User memory: %zu KB.\n", userMemoryUsage);

    size_t peakUserMemoryUsage = Applications_GetPeakUserModeMemoryUsageInKB();
    Log_Debug("Peak user memory: %zu KB.\n", peakUserMemoryUsage);

    size_t totalMemoryUsage = Applications_GetTotalMemoryUsageInKB();
    Log_Debug("Total memory: %zu KB.\n", totalMemoryUsage);
}

/// <summary>
///     Allocates or frees memory based on the list size.
/// </summary>
static void AllocateMemoryTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AllocateMemoryTimer_Consume;
        return;
    }

    PrintMemoryUsage();

    if (listSize == MAX_NUMBER_NODES) {
        Log_Debug(
            "\nFreeing the initial memory buffer and the list, before allocating more memory.\n");

        // Note that although we free the allocation here, there is no guarantee the C runtime will
        // return the allocation to the OS, and so there will not necessarily be a reduction in user
        // memory use
        free(userMemoryBuffer);
        userMemoryBuffer = NULL;

        // Because in the DeleteList function the sockets (which consume kernel
        // memory) are closed, the total memory usage will decrease
        DeleteList(&linkedListHead);

        // Display memory usage after freeing resources
        PrintMemoryUsage();
    }

    AppendNode(&linkedListHead);
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

    struct timespec memoryAllocatorInterval = {.tv_sec = 0, .tv_nsec = 250 * 1000 * 1000};
    allocateMemoryTimer = CreateEventLoopPeriodicTimer(eventLoop, &AllocateMemoryTimerEventHandler,
                                                       &memoryAllocatorInterval);

    if (allocateMemoryTimer == NULL) {
        Log_Debug("ERROR: Could not create memory timer.\n");
        return ExitCode_Init_AllocateMemoryTimer;
    }

    Log_Debug("Before allocating a buffer...\n");
    PrintMemoryUsage();

    // Allocate and zero-fill the buffer (to ensure the allocated pages are committed)
    userMemoryBuffer = (int *)calloc(USER_MEMORY_BUFFER_NO_ELEM, sizeof(int));
    if (userMemoryBuffer == NULL) {
        Log_Debug("ERROR: Could not initialize the buffer %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_Init_AllocateMemoryBuffer;
    }
    Log_Debug("\nAllocating a buffer of %zu elements * %zu bytes.\n", USER_MEMORY_BUFFER_NO_ELEM,
              sizeof(int));

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(allocateMemoryTimer);
    EventLoop_Close(eventLoop);
    free(userMemoryBuffer);
    DeleteList(&linkedListHead);
    Log_Debug("Closing file descriptors.\n");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Memory usage application starting.\n");
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
    return exitCode;
}