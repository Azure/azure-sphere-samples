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
    ExitCode_Main_EventLoopFail = 2,
    ExitCode_ButtonTimer_Consume = 3,
    ExitCode_AzureTimer_Consume = 4,
    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_Button = 6,
    ExitCode_Init_Led = 7,
    ExitCode_Init_ButtonPollTimer = 8,
    ExitCode_Init_AzureTimer = 9,
    ExitCode_IsButtonPressed_GetValue = 10,

    ExitCode_Validate_ScopeId = 11,
    ExitCode_Validate_Hostname = 12,
    ExitCode_Validate_IoTEdgeCAPath = 13,

    ExitCode_InterfaceConnectionStatus_Failed = 14,

    ExitCode_IoTEdgeRootCa_Open_Failed = 15,
    ExitCode_IoTEdgeRootCa_LSeek_Failed = 16,
    ExitCode_IoTEdgeRootCa_FileSize_Invalid = 17,
    ExitCode_IoTEdgeRootCa_FileSize_TooLarge = 18,
    ExitCode_IoTEdgeRootCa_FileRead_Failed = 19,

    ExitCode_PayloadSize_TooLarge = 20,

    ExitCode_Init_TelemetryTimer = 21,
    ExitCode_TelemetryTimer_Consume = 22,

    ExitCode_Validate_ConnectionConfig = 23,
    ExitCode_Connection_CreateTimer = 24,
    ExitCode_Connection_TimerStart = 25,
    ExitCode_Connection_TimerConsume = 26,
    ExitCode_Connection_InitializeClient = 27,

    ExitCode_ReadSensorTimer_Consume = 28,

    ExitCode_Init_ButtonA = 29,
    ExitCode_Init_ButtonB = 30,
    ExitCode_Init_RebootTimer = 31,
    ExitCode_Init_DirectMethods = 32,
    ExitCode_Init_StatusLeds = 33,

    ExitCode_Validate_ConnectionType = 34,

    ExitCode_Init_sensorPollTimer = 35,
    ExitCode_Init_TelemetrytxIntervalr = 36,

    ExitCode_SetGPIO_Failed = 37,
    ExitCode_Button_Telemetry_Malloc_Failed = 38,
    ExitCode_RebootDevice_Malloc_failed = 39,
    ExitCode_SettxInterval_Malloc_failed = 40,
    ExitCode_DirectMethodError_Malloc_failed = 41,
    ExitCode_DirectMethodResponse_Malloc_failed = 42,
    ExitCode_DirectMethod_InvalidPayload_Malloc_failed = 43,
    ExitCode_DirectMethod_RebootExectued = 44,
    ExitCode_Init_OledUpdateTimer = 45,

    // IoTConnect exit codes
    ExitCode_IoTCTimer_Consume = 46,
    ExitCode_Init_IoTCTimer = 47,
    ExitCode_IoTCMalloc_Failed = 48,
   
    // M4 intercore comms exit codes
    ExitCode_Init_Invalid_Number_Real_Time_Apps = 49,
    ExitCode_Init_Open_Socket = 50,
    ExitCode_Init_RegisterIo = 51,
    ExitCode_Init_Rt_PollTimer = 51,
    ExitCode_Read_RT_Socket = 52,
    ExitCode_Write_RT_Socket = 53,
    ExitCode_RT_Timer_Consume = 54,

    // Reboot exit codes
    ExitCode_TriggerReboot_Success = 55,
    ExitCode_UpdateCallback_Reboot = 56,

    // Telemetry Resend exit code
    ExitCode_AddTelemetry_Malloc_Failed = 57,


} ExitCode;

/// <summary>
/// Callback type for a function to be invoked when a fatal error with exit code needs to be raised.
/// </summary>
/// <param name="exitCode">Exit code representing the failure.</param>
typedef void (*ExitCode_CallbackType)(ExitCode exitCode);
