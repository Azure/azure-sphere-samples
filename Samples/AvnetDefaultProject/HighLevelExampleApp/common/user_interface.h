/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdbool.h>

#include <applibs/eventloop.h>
#include "exitcodes.h"
#include "azure_iot.h"
#include "build_options.h"
#include <applibs/applications.h>

typedef enum {
    UserInterface_Button_A,
    UserInterface_Button_B,
} UserInterface_Button;

#if (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))
#define RGB_LED1_INDEX 0
#define RGB_LED2_INDEX 1
#define RGB_LED3_INDEX 2
// Define which LED to light up for each case
typedef enum {
    RGB_No_Connections = 0b000,
    RGB_No_Network = 0b001,        // No WiFi connection
    RGB_Network_Connected = 0b010, // Connected to Azure, not IoT Hub
    RGB_IoT_Hub_Connected = 0b100, // Connected to IoT Hub
} RGB_Status;

void updateConnectionStatusLed(void);
void setConnectionStatusLed(RGB_Status networkStatus);
#endif // (defined(USE_SK_RGB_FOR_IOT_HUB_CONNECTION_STATUS) && defined(IOT_HUB_APPLICATION))

/// <summary>
/// Callback for a function to be invoked when a button is pressed.
/// </summary>
/// <param name="buttonPressed">A <see cref="UserInterface_Button" /> value indicating which
/// button was pressed.</param>
typedef void (*UserInterface_ButtonPressedCallbackType)(UserInterface_Button buttonPressed);

/// <summary>
/// Initialize the user interface.
/// </summary>
/// <param name="el">Pointer to an EventLoop to which events can be registered.</param>
/// <param name="buttonPressed">Function to be called when a button is pressed.</param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode UserInterface_Initialise(EventLoop *el,
                                  UserInterface_ButtonPressedCallbackType buttonPressed,
                                  ExitCode_CallbackType failureCallback);

/// <summary>
/// Close and clean up the user interface.
/// </summary>
/// <param name=""></param>
void UserInterface_Cleanup(void);

/// <summary>
/// Set the status of the status LED.
/// </summary>
/// <param name="status">Status LED status.</param>
void UserInterface_SetStatus(bool status);

void checkMemoryUsageHighWaterMark(void);
