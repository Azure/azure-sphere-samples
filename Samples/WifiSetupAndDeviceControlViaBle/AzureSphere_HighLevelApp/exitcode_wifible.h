/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ButtonTimer_Consume = 2,
    ExitCode_ButtonTimer_GetEvent1 = 3,
    ExitCode_ButtonTimer_GetEvent2 = 4,

    ExitCode_Init_ResetPin = 5,
    ExitCode_Init_Epoll = 6,
    ExitCode_Init_Uart = 7,
    ExitCode_Init_Button1 = 8,
    ExitCode_Init_Button2 = 9,
    ExitCode_Init_ButtonTimer = 10,
    ExitCode_Init_BondedDevicesLed = 11,
    ExitCode_Init_AllDevicesLed = 12,
    ExitCode_Init_BleConnectedLed = 13,
    ExitCode_Init_DeviceControlLed = 14,

    ExitCode_Main_EventCall = 15,

    ExitCode_MsgProtoInit_UartHandler = 16,
    ExitCode_MsgProtoInit_Timer = 17
} ExitCode;
