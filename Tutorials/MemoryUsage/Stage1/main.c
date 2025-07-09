/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This C tutorial for Azure Sphere demonstrates debugging an application
// using the Visual Studio memory usage chart. The application allocates
// memory and adds a node to a linked list when pressing button A and
// deletes the last node from the list when pressing button B. This application
// has an intentional memory leak. The application will be killed after
// multiple allocations. Stage2 provides the correct implementation.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (functionality for interacting with GPIOs)
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for IO events)
// - curl (URL transfer library)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/eventloop.h>

#include <curl/curl.h>

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
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Init_EventLoop = 2,
    ExitCode_Init_ButtonAddNode = 3,
    ExitCode_Init_ButtonDeleteNode = 4,
    ExitCode_Init_ButtonPollTimer = 5,

    ExitCode_Main_EventLoopFail = 6,
    ExitCode_ButtonTimer_GetButtonState = 7,
    ExitCode_ButtonTimer_Consume = 8,

    ExitCode_AddNode_AllocateUserData = 9,
    ExitCode_AddNode_CreateNode = 10,

    ExitCode_Init_CurlGlobalInit = 11,
    ExitCode_Init_CurlHandleInit = 12

} ExitCode;

// File descriptors - initialized to invalid value
static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;
static int appendNodeButtonGpioFd = -1;
static int deleteNodeButtonGpioFd = -1;
CURL *curlHandle = NULL;

// Button state variables
static GPIO_Value_Type buttonAddNodeState = GPIO_Value_High;
static GPIO_Value_Type buttonDeleteNodeState = GPIO_Value_High;

typedef struct Node {
    int *userData;
    struct Node *next;
} Node;
static Node *linkedListHead = NULL;
static void PushNode(Node **headNode);
static void DeleteLastNode(Node **headNode);
static void DeleteList(Node **headNode);

static const size_t NUM_ELEMS = 5000;
static unsigned int listSize = 0;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void CheckButtonAddNode(void);
static void CheckButtonDeleteLastNode(void);

static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);

static void TerminationHandler(int signalNumber);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
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
///     Adds a node to the linked list.
/// </summary>
static void PushNode(Node **headNode)
{
    struct Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        Log_Debug("ERROR: couldn't allocate memory %s (%d)\n", strerror(errno), errno);
        exitCode = ExitCode_AddNode_CreateNode;
        return;
    }

    // Allocate and initialize the memory
    newNode->userData = (int *)calloc(NUM_ELEMS, sizeof(int));
    if (newNode->userData == NULL) {
        Log_Debug("ERROR: couldn't allocate and initialize memory %s (%d)\n", strerror(errno),
                  errno);
        exitCode = ExitCode_AddNode_AllocateUserData;
        free(newNode);
        return;
    }

    listSize++;
    Log_Debug("\nAdding a node to the linked list (list size = %u).\n", listSize);

    if (*headNode == NULL) {
        newNode->next = NULL;
        *headNode = newNode;
        return;
    }

    // The next of the new node is the head of the list
    newNode->next = (*headNode);

    // Move the head to point to the new node
    *headNode = newNode;
}

/// <summary>
///     Erases the last node from the list.
/// </summary>
static void DeleteLastNode(Node **headNode)
{
    if (*headNode == NULL) {
        Log_Debug("\nThe list is empty...\n");
        return;
    }

    listSize--;
    Log_Debug("\nDeleting the last node from the linked list (list size = %u).\n", listSize);

    // If the list contains just one node, delete it
    if ((*headNode)->next == NULL) {
        free(*headNode);
        *headNode = NULL;
        return;
    }

    // Find the second last node
    Node *secondLast = *headNode;
    while (secondLast->next->next != NULL) {
        secondLast = secondLast->next;
    }

    free(secondLast->next);

    secondLast->next = NULL;
}

/// <summary>
///     Erases the linked list.
/// </summary>
static void DeleteList(Node **headNode)
{
    Log_Debug("Delete the linked list.\n");
    Node *currentNode = *headNode;

    while (currentNode != NULL) {
        Node *nextNode = currentNode->next;
        free(currentNode->userData);
        free(currentNode);
        currentNode = nextNode;
    }

    *headNode = NULL;
    listSize = 0;
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonState;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

/// <summary>
///     Button timer event:  Check the status of the buttons.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }
    CheckButtonAddNode();
    CheckButtonDeleteLastNode();
}

/// <summary>
///     If the button is pressed, add a node to the linked list. The button
///     has GPIO_Value_Low when pressed and GPIO_Value_High when released.
/// </summary>
static void CheckButtonAddNode(void)
{
    if (IsButtonPressed(appendNodeButtonGpioFd, &buttonAddNodeState)) {
        PushNode(&linkedListHead);
    }
}

/// <summary>
///     Handle button timer event: if the button is pressed erase a node from the list.
///     The button has GPIO_Value_Low when pressed and GPIO_Value_High when released.
/// </summary>
static void CheckButtonDeleteLastNode(void)
{
    if (IsButtonPressed(deleteNodeButtonGpioFd, &buttonDeleteNodeState)) {
        DeleteLastNode(&linkedListHead);
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

    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    appendNodeButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (appendNodeButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonAddNode;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    deleteNodeButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (deleteNodeButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonDeleteNode;
    }

    // Set up a timer to poll the buttons
    struct timespec buttonPressCheckPeriod1Ms = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler,
                                                   &buttonPressCheckPeriod1Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // Init the cURL library in order to demonstrate tracking of shared library heap memory usage
    CURLcode res = 0;
    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        Log_Debug(" (curl err=%d, '%s')\n", res, curl_easy_strerror(res));
        return ExitCode_Init_CurlGlobalInit;
    }

    if ((curlHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed\n");
        return ExitCode_Init_CurlHandleInit;
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
    DeleteList(&linkedListHead);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(appendNodeButtonGpioFd, "AddNodeButtonGpioFd");
    CloseFdAndPrintError(deleteNodeButtonGpioFd, "DeleteNodeButtonGpioFd");

    curl_easy_cleanup(curlHandle);
    curl_global_cleanup();
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