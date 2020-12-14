#ifndef	EXIT_CODES_C
#define EXIT_CODES_C

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
    ExitCode_Init_MessageButton = 6,
    ExitCode_Init_OrientationButton = 7,
    ExitCode_Init_TwinStatusLed = 8,
    ExitCode_Init_ButtonPollTimer = 9,
    ExitCode_Init_AzureTimer = 10,

    ExitCode_IsButtonPressed_GetValue = 11,

    ExitCode_Validate_ConnectionType = 12,
    ExitCode_Validate_ScopeId = 13,
    ExitCode_Validate_Hostname = 14,
    ExitCode_Validate_IoTEdgeCAPath = 15,

    ExitCode_InterfaceConnectionStatus_Failed = 16,

    ExitCode_IoTEdgeRootCa_Open_Failed = 17,
    ExitCode_IoTEdgeRootCa_LSeek_Failed = 18,
    ExitCode_IoTEdgeRootCa_FileSize_Invalid = 19,
    ExitCode_IoTEdgeRootCa_FileSize_TooLarge = 20,
    ExitCode_IoTEdgeRootCa_FileRead_Failed = 21,
    ExitCode_PayloadSize_TooLarge = 22,

    // IoTConnect Specific Error Codes
    
    // Mutable Storage exit codes
    ExitCode_WriteFile_OpenMutableFile = 23,
    ExitCode_WriteFile_Write = 24,
    ExitCode_ReadFile_OpenMutableFile = 25,
    ExitCode_ReadFile_Read = 26,
    ExitCode_SendTelemetryMemoryError = 27,

    ExitCode_IoTCTimer_Consume = 28,
    ExitCode_Init_IoTCTimer = 29,
    ExitCode_IoTCMalloc_Failed = 30
} ExitCode;

#endif 