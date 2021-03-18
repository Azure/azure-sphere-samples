#ifndef _EXIT_CODES_H
#define _EXIT_CODES_H

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
    ExitCode_ReadSensorTimer_Consume = 4,
    ExitCode_TelemetryTimer_Consume = 5,
    ExitCode_AzureTimer_Consume = 6,

    ExitCode_Init_EventLoop = 7,
    ExitCode_Init_ButtonA = 8,
    ExitCode_Init_ButtonB = 9,
    ExitCode_Init_ButtonPollTimer = 10,
    ExitCode_Init_RebootTimer = 11,
    ExitCode_Init_AzureTimer = 12,
    ExitCode_Init_DirectMethods = 13,
    ExitCode_Init_StatusLeds = 14,

    ExitCode_IsButtonPressed_GetValue = 15,

    ExitCode_Validate_ConnectionType = 16,
    ExitCode_Validate_ScopeId = 17,
    ExitCode_Validate_Hostname = 18,
    ExitCode_Validate_IoTEdgeCAPath = 19,
    ExitCode_InterfaceConnectionStatus_Failed = 20,

    ExitCode_IoTEdgeRootCa_Open_Failed = 21,
    ExitCode_IoTEdgeRootCa_LSeek_Failed = 22,
    ExitCode_IoTEdgeRootCa_FileSize_Invalid = 23,
    ExitCode_IoTEdgeRootCa_FileSize_TooLarge = 24,
    ExitCode_IoTEdgeRootCa_FileRead_Failed = 25,

    ExitCode_PayloadSize_TooLarge = 26,
    ExitCode_Init_sensorPollTimer = 27,
    ExitCode_Init_TelemetrytxIntervalr = 28,

    ExitCode_SetGPIO_Failed = 29,
    ExitCode_Button_Telemetry_Malloc_Failed = 230,
    ExitCode_RebootDevice_Malloc_failed = 31,
    ExitCode_SettxInterval_Malloc_failed = 32,
    ExitCode_DirectMethodError_Malloc_failed = 33,
    ExitCode_DirectMethodResponse_Malloc_failed = 34,
    ExitCode_DirectMethod_InvalidPayload_Malloc_failed = 35,
    ExitCode_DirectMethod_RebootExectued = 36,
    ExitCode_Init_OledUpdateTimer = 37,

    // IoTConnect exit codes
    ExitCode_IoTCTimer_Consume = 38,
    ExitCode_Init_IoTCTimer = 39,
    ExitCode_IoTCMalloc_Failed = 40,
   
    // M4 intercore comms exit codes
    ExitCode_Init_Invalid_Number_Real_Time_Apps = 41,
    ExitCode_Init_Open_Socket = 42,
    ExitCode_Init_RegisterIo = 43,
    ExitCode_Init_Rt_PollTimer = 44,
    ExitCode_Read_RT_Socket = 45,
    ExitCode_Write_RT_Socket = 46,
    ExitCode_RT_Timer_Consume = 47,

    // Reboot exit codes
    ExitCode_TriggerReboot_Success = 48,
    ExitCode_UpdateCallback_Reboot = 49,

} ExitCode;



#endif 