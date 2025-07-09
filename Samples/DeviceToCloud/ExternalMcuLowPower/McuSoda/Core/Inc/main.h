/* USER CODE BEGIN Header */
/* Portions copyright Microsoft. */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
_Noreturn void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc;

// The GPIO which wakes up the MT3620 is held low for this amount of time.
// This must be less than DEBOUNCE_PERIOD_MS, else the main loop in soda.c
// will go into WFI without systick enabled before the period ends.
#define TO_MT3620_WAKEUP_PERIOD_MS	10

// When the user presses the dispense button and the machine can dispense an
// item (because it is not empty), switch on LED 3 for this amount of time.
#define DISPENSE_LED_PERIOD_MS		(3 * 1000)

// Next time in ticks when it is safe to sleep.
extern __IO uint32_t nextSleepTimeTicks;

typedef struct {
	// Maximum number of units this machine can hold.
	const int machineCapacity;
	// When the number of units reaches this value, alert the MT3620.
	const int alertThreshold;
	// Total number of units which have been added to this machine.
	// (Not the current number of units in the machine.)
	int stockedDispenses;
	// Total number of units which have been dispensed.
	int issuedDispenses;
} MachineState;

extern MachineState state;

_Noreturn void RunSodaMachine(void);
void ReadMessageAsync(void);

void HandleWakeupFromMT3620(void);
void HandleButtonPress(void);
void HandleMessage(void);
void StopBlinkingFlavorLed(uint32_t now);
void StopWakingUpMT3620(uint32_t now);

void SetFlavor(bool r, bool g, bool b);
void SetFlavorLedEnabled(bool enabled);

void RestoreStateFromFlash(void);
void WriteLatestMachineState(void);

float ReadBatteryLevel(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MT3620_TO_MCU_WAKEUP_Pin GPIO_PIN_0
#define MT3620_TO_MCU_WAKEUP_GPIO_Port GPIOA
#define MT3620_TO_MCU_WAKEUP_EXTI_IRQn EXTI0_1_IRQn
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_3
#define VCP_RX_GPIO_Port GPIOA
#define ADC_BATTERY_LVL_Pin GPIO_PIN_4
#define ADC_BATTERY_LVL_GPIO_Port GPIOA
#define DISPENSE_Pin GPIO_PIN_8
#define DISPENSE_GPIO_Port GPIOA
#define DISPENSE_EXTI_IRQn EXTI4_15_IRQn
#define ENA_BATTERY_LVL_Pin GPIO_PIN_9
#define ENA_BATTERY_LVL_GPIO_Port GPIOA
#define RESTOCK_Pin GPIO_PIN_11
#define RESTOCK_GPIO_Port GPIOA
#define RESTOCK_EXTI_IRQn EXTI4_15_IRQn
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define LD3_Pin GPIO_PIN_3
#define LD3_GPIO_Port GPIOB
#define MCU_TO_MT3620_WAKEUP_Pin GPIO_PIN_4
#define MCU_TO_MT3620_WAKEUP_GPIO_Port GPIOB
#define TRILED_R_Pin GPIO_PIN_5
#define TRILED_R_GPIO_Port GPIOB
#define TRILED_G_Pin GPIO_PIN_6
#define TRILED_G_GPIO_Port GPIOB
#define TRILED_B_Pin GPIO_PIN_7
#define TRILED_B_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
