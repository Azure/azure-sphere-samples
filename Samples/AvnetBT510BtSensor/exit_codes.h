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
    ExitCode_IpAddressTimer_Consume = 3,
    ExitCode_AzureTimer_Consume = 4,
    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_MessageButton = 6,
    ExitCode_Init_OrientationButton = 7,
    ExitCode_Init_StatusLeds = 8,
    ExitCode_Init_AzureTimer = 9,
    ExitCode_IsButtonPressed_GetValue = 10,
    ExitCode_Validate_ConnectionType = 11,
    ExitCode_Validate_ScopeId = 12,
    ExitCode_Validate_IotHubHostname = 13,
    ExitCode_Validate_DeviceId = 14,
    ExitCode_InterfaceConnectionStatus_Failed = 15,
    ExitCode_Init_UartOpen = 16,
    ExitCode_Init_RegisterIo = 17,
    ExitCode_UartEvent_Read = 18,
    ExitCode_SendMessage_Write = 19,
    ExitCode_UartBuffer_Overflow = 20,
    ExitCode_Init_nRF_Reset = 21,

    // IoTConnect Specific Error Codes
    
    // Mutable Storage exit codes
    ExitCode_WriteFile_OpenMutableFile = 22,
    ExitCode_WriteFile_Write = 23,
    ExitCode_ReadFile_OpenMutableFile = 24,
    ExitCode_ReadFile_Read = 25,
    ExitCode_SendTelemetryMemoryError = 26,

    ExitCode_IoTCTimer_Consume = 27,
    ExitCode_Init_IoTCTimer = 28,
    ExitCode_IoTCMalloc_Failed = 29
} ExitCode;

#endif 