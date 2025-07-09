/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
///     Termination codes for this application. These are used for the
///     application exit code. They must all be between zero and 255,
///     where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Init_EventLoop,
    ExitCode_Init_CopyScopeId,
    ExitCode_Validation_ScopeId,

    ExitCode_BusinessLogic_TimeoutTimerCreate,
    ExitCode_BusinessLogic_SetTimeoutTimer,

    ExitCode_Uart_Init_OpenFail,
    ExitCode_Uart_Init_EventRegisterFail,

    ExitCode_MsgProtoInit_Timer,

    ExitCode_Main_EventLoopFail,

    ExitCode_McuMessaging_Timeout,

    ExitCode_AzureIoT_Init_InvalidScopeId,
    ExitCode_AzureTimer_Consume,
    ExitCode_IsNetworkingReady_Failed,
    ExitCode_Init_AzureTimer,

    ExitCode_Cloud_Init_DeviceTwinCallback,

    ExitCode_Update_Init_NoUpdateEvent,
    ExitCode_Update_Init_CreateWaitForUpdatesCheckTimer,
    ExitCode_Update_Init_SetWaitForUpdatesCheckTimer,
    ExitCode_Update_Init_CreateWaitForUpdatesDownloadTimer,
    ExitCode_Update_UpdatesStarted_SetWaitForUpdatesDownloadTimer,

    ExitCode_Update_UpdateCallback_GetUpdateData,
    ExitCode_Update_UpdateCallback_DeferEvent,
    ExitCode_Update_UpdateCallback_UnexpectedStatus
} ExitCode;

typedef void (*ExitCodeCallbackType)(ExitCode);
