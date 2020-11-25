/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

// This file defines the mapping from the Avnet MT3620 Starter Kit (SK) to the
// 'sample hardware' abstraction used by the samples at https://github.com/Azure/azure-sphere-samples.
// Some peripherals are on-board on the Avnet MT3620 SK, while other peripherals must be attached externally if needed.
// See https://aka.ms/AzureSphereHardwareDefinitions for more information on how to use hardware abstractions,
// to enable apps to work across multiple hardware variants.

// This file is autogenerated from ../../sample_appliance.json.  Do not edit it directly.

#pragma once
#include "avnet_mt3620_sk_rev2.h"

// MT3620 SK: User Button A.
#define SAMPLE_BUTTON_1 AVNET_MT3620_SK_USER_BUTTON_A

// MT3620 SK: User Button B.
#define SAMPLE_BUTTON_2 AVNET_MT3620_SK_USER_BUTTON_B

// MT3620 SK: PWM LED controller
#define SAMPLE_LED_PWM_CONTROLLER AVNET_MT3620_SK_PWM_CONTROLLER2

// MT3620 SK: Channel 1 for the PWM LED1 green. In the app manifest, it is only necessary to request the capability for the PWM Controller, SAMPLE_LED_PWM_CONTROLLER.
#define SAMPLE_LED_PWM_CHANNEL MT3620_PWM_CHANNEL1

// MT3620 SK: User LED.
#define SAMPLE_LED AVNET_MT3620_SK_USER_LED_RED

// MT3620 SK: ADC Potentiometer controller
#define SAMPLE_POTENTIOMETER_ADC_CONTROLLER AVNET_MT3620_SK_ADC_CONTROLLER0

// MT3620 SK: Connect external potentiometer to ADC controller 0, channel 1 using CLICK1 AN. In the app manifest, it is only necessary to request the capability for the ADC Group Controller, SAMPLE_POTENTIOMETER_ADC_CONTROLLER.
#define SAMPLE_POTENTIOMETER_ADC_CHANNEL MT3620_ADC_CHANNEL1

// MT3620 SK: User LED RED Channel.
#define SAMPLE_RGBLED_RED AVNET_MT3620_SK_USER_LED_RED

// MT3620 SK: User LED GREEN Channel.
#define SAMPLE_RGBLED_GREEN AVNET_MT3620_SK_USER_LED_GREEN

// MT3620 SK: User LED BLUE Channel.
#define SAMPLE_RGBLED_BLUE AVNET_MT3620_SK_USER_LED_BLUE

// MT3620 SK: Connect CLICK1 RX (RX) to CLICK1 TX (TX).
#define SAMPLE_UART_LOOPBACK AVNET_MT3620_SK_ISU0_UART

// MT3620 SK: Connect external LSM6DS3 to I2C using CLICK1, pin MISO (SDA) and pin MOSI (SCL).
#define SAMPLE_LSM6DS3_I2C AVNET_MT3620_SK_ISU1_I2C

// MT3620 SK: Connect external LSM6DS3 to SPI using CLICK1, pin MISO (MISO), pin SCK (SCLK), pin MOSI (MOSI) and CLICK2 pin CS (CSB).
#define SAMPLE_LSM6DS3_SPI AVNET_MT3620_SK_ISU0_SPI

// MT3620 SPI Chip Select (CS) value "B". This is not a peripheral identifier, and so has no meaning in an app manifest.
#define SAMPLE_LSM6DS3_SPI_CS MT3620_SPI_CS_B

// MT3620 SK: Connect external NRF52 RESET signal using CLICK1 (PWM).
#define SAMPLE_NRF52_RESET AVNET_MT3620_SK_GPIO0

// MT3620 SK: Connect external NRF52 DFU signal using CLICK1 (AN).
#define SAMPLE_NRF52_DFU AVNET_MT3620_SK_GPIO42

// MT3620 SK: Connect external NRF52 UART using CLICK1: RX (RX), TX (TX), CTS (CS), and RTS (SDI).
#define SAMPLE_NRF52_UART AVNET_MT3620_SK_ISU0_UART

// MT3620 SK: Connect external STM32 UART using CLICK1: RX (RX), TX (TX), CTS (CS), and RTS (SDI).
#define SAMPLE_STM32_UART AVNET_MT3620_SK_ISU0_UART

// MT3620 SK: Connect PMOD to un-populated PMOD connector
#define SAMPLE_PMOD_UART AVNET_MT3620_SK_ISU1_UART

// Pin7 on the PMOD connector
#define SAMPLE_PMOD_PIN7 AVNET_MT3620_SK_GPIO39

// Pin8 on the PMOD connector
#define SAMPLE_PMOD_PIN8 AVNET_MT3620_SK_GPIO35

// Pin9 on the PMOD connector
#define SAMPLE_PMOD_PIN9 AVNET_MT3620_SK_GPIO1

// MT3620 SK: Connect external red LED using CLICK1, pin PWM.
#define SAMPLE_DEVICE_STATUS_LED AVNET_MT3620_SK_GPIO0

// MT3620 SK: User APP LED (Yellow).
#define SAMPLE_PENDING_UPDATE_LED AVNET_MT3620_SK_APP_STATUS_LED_YELLOW

// MT3620 SK: LSM6DSO accelerometer.
#define SAMPLE_LSM6DSO_I2C AVNET_AESMS_ISU2_I2C

// Connect to Click 1: Header 2: Pin1
#define ARDUCAM_CS AVNET_MT3620_SK_GPIO0

// Connect to Click 1: Header 2: Pin 6(SDA) and Pin5(SCL)
#define ARDUCAM_I2C AVNET_MT3620_SK_ISU2_I2C

// Connect to Click 1: Header 1: Pin 5(MISO), Pin6(MOSI)
#define ARDUCAM_SPI AVNET_MT3620_SK_ISU0_SPI

// Connect to Click 1: Header 2: Pin2 (INT)
#define PWM_SERVO_CONTROLLER AVNET_MT3620_SK_PWM_CONTROLLER1

// Insert into Click 2: Header 2
#define LOAD_CELL_2_CLICK_I2C AVNET_MT3620_SK_ISU2_I2C

// Insert into Click 2: Header 2.
#define LOAD_CELL_2_CLICK_DATA_READY AVNET_MT3620_SK_GPIO34

