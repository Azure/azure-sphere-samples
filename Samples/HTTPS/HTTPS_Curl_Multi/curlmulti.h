/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This header file defines the values which are used in multiple source files.

#pragma once

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Init_EventLoop = 2,

    ExitCode_Main_EventLoopFail = 3,

    ExitCode_UiInit_SampleLed = 4,
    ExitCode_UiInit_BlinkTimer = 5,
    ExitCode_UiInit_Button = 6,
    ExitCode_UiInit_ButtonPollTimer = 7,

    ExitCode_WebClientInit_CurlTimer = 8,

    ExitCode_CurlInit_GlobalInit = 9,
    ExitCode_CurlInit_MultiInit = 10,
    ExitCode_CurlInit_MultiSetOptSocketFunction = 11,
    ExitCode_CurlInit_MultiSetOptTimerFunction = 12,

    ExitCode_CurlSetupEasy_EasyInit = 13,
    ExitCode_CurlSetupEasy_OptUrl = 14,
    ExitCode_CurlSetupEasy_OptFollowLocation = 15,
    ExitCode_CurlSetupEasy_OptProtocols = 16,
    ExitCode_CurlSetupEasy_OptRedirProtocols = 17,
    ExitCode_CurlSetupEasy_OptWriteFunction = 18,
    ExitCode_CurlSetupEasy_OptWriteData = 19,
    ExitCode_CurlSetupEasy_OptHeaderData = 20,
    ExitCode_CurlSetupEasy_OptUserAgent = 21,
    ExitCode_CurlSetupEasy_StoragePath = 22,
    ExitCode_CurlSetupEasy_CAInfo = 23,
    ExitCode_CurlSetupEasy_Verbose = 24,
    ExitCode_CurlSetupEasy_CurlSetDefaultProxy = 25
} ExitCode;
