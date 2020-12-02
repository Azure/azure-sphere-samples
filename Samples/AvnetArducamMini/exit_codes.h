#ifndef __EXIT_CODES_H
#define __EXIT_CODES_H

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_LedTimer_Consume = 2,
    ExitCode_LedTimer_SetLedState = 3,

    ExitCode_ButtonTimer_Consume = 4,
    ExitCode_ButtonTimer_GetButtonState = 5,
    ExitCode_ButtonTimer_SetBlinkPeriod = 6,

    ExitCode_Init_EventLoop = 7,
    ExitCode_Init_Button = 8,
    ExitCode_Init_ButtonPollTimer = 9,
    ExitCode_Init_Led = 10,
    ExitCode_Init_LedBlinkTimer = 11,

    ExitCode_Main_EventLoopFail = 12,

    ExitCode_Arducam_Init_Error = 13,
    ExitCode_Arducam_Not_Found = 14,
    ExitCode_Arducam_GPIO_Init_Failed = 15,
    ExitCode_Arducam_SPI_Init_Failed = 16,
    ExitCode_Arducam_I2C_Init_Failed = 17

} ExitCode;


#endif 