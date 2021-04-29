/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include <applibs/eventloop.h>
#include "exitcodes.h"

typedef enum {
    UserInterface_Button_A,
    UserInterface_Button_B,
} UserInterface_Button;

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
