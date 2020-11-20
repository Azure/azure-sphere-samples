/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use Pulse Width
// Modulation (PWM).
// The sample opens a PWM controller. Adjusting the duty cycle will change the
// brightness of an LED.
//
// It uses the API for the following Azure Sphere application libraries:
// - pwm (Pulse Width Modulation)
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for timer events)

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/pwm.h>
#include <applibs/eventloop.h>

#include "servo.h"

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_DIRECTORY argument in the call to
// azsphere_target_hardware_definition to "HardwareDefinitions/avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
#include <hw/sample_appliance.h>

// This sample uses a single-thread event loop pattern.
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm = 1,
    ExitCode_TurnOffChannel_Apply = 2,
    ExitCode_StepTimerHandler_Consume = 3,
    ExitCode_StepTimerHandler_Apply = 4,
    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_StepTimer = 6,
    ExitCode_Init_PwmOpen = 7,
    ExitCode_Main_EventLoopFail = 8, 
    ExitCode_ButtonTimer_GetButtonAState = 9,
    ExitCode_ButtonTimer_GetButtonBState = 10,
    ExitCode_ButtonTimer_Consume = 11,
    ExitCode_Init_ButtonA = 12,
    ExitCode_Init_ButtonB = 13,
    ExitCode_Init_ButtonPollTimer = 14
} ExitCode;

// File descriptors - initialized to invalid value
static int pwmServoFd = -1;
static int buttonAGpioFd = -1;
static int buttonBGpioFd = -1;

// GPIO variables to hold the "last" state of the button
static GPIO_Value_Type buttonAState = GPIO_Value_High;
static GPIO_Value_Type buttonBState = GPIO_Value_High;

// Timer for polling button presses
static EventLoopTimer *buttonPollTimer = NULL;

// Servo variables
struct _SERVO_State *myServo;
float myServoAngle = SERVO_STANDBY_ANGLE;

// Your servos might have slightly different duty cycles so you might want to edit the
// config values.
static const uint32_t periodNs = 20000000;
static const uint32_t maxDutyCycleNs = 2400000;
static const uint32_t minDutyCycleNs = 600000;
static const uint32_t minAngle = 0;
static const uint32_t maxAngle = 180;

static EventLoop *eventLoop = NULL;

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void TerminationHandler(int signalNumber);
static void ButtonTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);

/// <summary>
///		Initializes a servo
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
int InitServo(int pwmFd, unsigned int channel, struct _SERVO_State **servo, int minAngle,
              int maxAngle)
{
    struct SERVO_Config servoConfig;

    servoConfig.pwmFd = pwmFd;
    servoConfig.pwmChannel = channel;
    servoConfig.minAngleDeg = minAngle;
    servoConfig.maxAngleDeg = maxAngle;
    servoConfig.minPulseNs = minDutyCycleNs;
    servoConfig.maxPulseNs = maxDutyCycleNs;
    servoConfig.periodNs = periodNs;

    if (SERVO_Init(&servoConfig, servo) < 0) {
        Log_Debug("Error initializing servo 0\n");
        return -1;
    }

    return 0;
}

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Handle button timer event: if the button is pressed, change the LED blink rate.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    bool updateServo = false;

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for a buttonA press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(buttonAGpioFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read buttonA GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonAState;
        return;
    }

    // If the buttonA has just been pressed, move the servo +5 degrees
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonAState) {
        if (newButtonState == GPIO_Value_Low) {
            myServoAngle += (float)5.0;
            updateServo = true;
        }
        buttonAState = newButtonState;
    }

    // Check for a buttonB press
    result = GPIO_GetValue(buttonBGpioFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read buttonB GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetButtonBState;
        return;
    }

    // If the buttonB has just been pressed, move the servo +20 degrees
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonBState) {
        if (newButtonState == GPIO_Value_Low) {
            myServoAngle += (float)20.0;
            updateServo = true;
        }
        buttonBState = newButtonState;
    }

    // If the updateServo flag is set then update the PWM signal
    if (updateServo) {

        // Verify we don't go too far
        if (myServoAngle > SERVO_MAX_ANGLE) {
            myServoAngle = SERVO_MIN_ANGLE;
        }

        // Set the new angle
        SERVO_SetAngle(myServo, myServoAngle);
        
        Log_Debug("ButtonX pressed, setting new Servo Angle to %.2f\n", myServoAngle);
    }
}


/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

  	// Initialize the Servo
    pwmServoFd = PWM_Open(PWM_SERVO_CONTROLLER);
    if (pwmServoFd == -1) {
        Log_Debug(
            "Error opening PWM_CONTROLLER: %s (%d). Check that app_manifest.json "
            "includes the PWM used.\n",
            strerror(errno), errno);
        return -1;
    }
    InitServo(pwmServoFd, MT3620_PWM_CHANNEL1, &myServo, (int)minAngle, (int)maxAngle);
    SERVO_SetAngle(myServo, myServoAngle);

   struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000000};
    buttonPollTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ButtonTimerEventHandler, &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_1 as input.\n");
    buttonAGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
    if (buttonAGpioFd == -1) {
        Log_Debug("ERROR: Could not open BUTTON_A: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonA;
    }
 
    // Open SAMPLE_BUTTON_2 GPIO as input, and set up a timer to poll it
    Log_Debug("Opening SAMPLE_BUTTON_2 as input.\n");
    buttonBGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
    if (buttonBGpioFd == -1) {
        Log_Debug("ERROR: Could not open BUTTON_B: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_ButtonB;
    }

    return ExitCode_Success;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{

    Log_Debug("Closing file descriptors.\n");

    // Move the servo home, then close the Fd
    SERVO_SetAngle(myServo, SERVO_STANDBY_ANGLE);
    SERVO_Destroy(myServo);
    CloseFdAndPrintError(pwmServoFd, "PwmServoFd");

    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(buttonAGpioFd, "ButtonAGpio");
    CloseFdAndPrintError(buttonAGpioFd, "ButtonBGpio");

}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Starting Servo Sample\n");
    Log_Debug("ButtonA increment servo 5 degrees\n");
    Log_Debug("ButtonA increment servo 20 degrees\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");

    return exitCode;
}