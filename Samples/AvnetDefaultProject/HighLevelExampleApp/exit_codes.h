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

    ExitCode_IsButtonPressed_GetValue = 14,

    ExitCode_Validate_ConnectionType = 15,
    ExitCode_Validate_ScopeId = 16,
    ExitCode_Validate_Hostname = 17,
    ExitCode_Validate_IoTEdgeCAPath = 18,
    ExitCode_InterfaceConnectionStatus_Failed = 19,

    ExitCode_IoTEdgeRootCa_Open_Failed = 20,
    ExitCode_IoTEdgeRootCa_LSeek_Failed = 21,
    ExitCode_IoTEdgeRootCa_FileSize_Invalid = 22,
    ExitCode_IoTEdgeRootCa_FileSize_TooLarge = 23,
    ExitCode_IoTEdgeRootCa_FileRead_Failed = 24,

    ExitCode_PayloadSize_TooLarge = 25,
    ExitCode_Init_sensorPollTimer = 26,
    ExitCode_Init_TelemetrytxIntervalr = 27,

    ExitCode_SetGPIO_Failed = 28,
    ExitCode_Button_Telemetry_Malloc_Failed = 29,
    ExitCode_RebootDevice_Malloc_failed = 30,
    ExitCode_SettxInterval_Malloc_failed = 31,
    ExitCode_DirectMethodError_Malloc_failed = 32,
    ExitCode_DirectMethodResponse_Malloc_failed = 33,
    ExitCode_DirectMethod_InvalidPayload_Malloc_failed = 34,
    ExitCode_DirectMethod_RebootExectued = 35,
    ExitCode_Init_OledUpdateTimer = 36,

    // IoTConnect exit codes
    ExitCode_IoTCTimer_Consume = 37,
    ExitCode_Init_IoTCTimer = 38,
    ExitCode_IoTCMalloc_Failed = 39,
   
    // M4 intercore comms exit codes
    ExitCode_Init_RegisterIo = 40,
    ExitCode_Init_Rt_PollTimer = 41,
    ExitCode_Read_RT_Socket = 42,
    ExitCode_Write_RT_Socket = 43,
    ExitCode_RT_Timer_Consume = 44,

    // Reboot exit codes
    ExitCode_TriggerReboot_Success = 45,
    ExitCode_UpdateCallback_Reboot = 46,

} ExitCode;



#endif 