/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

// This file defines the mapping from the Avnet MT3620 Starter Kit (SK) to the
// 'sample appliance' abstraction used by the samples at https://github.com/Azure/azure-sphere-samples.
// Some peripherals are on-board on the Avnet MT3620 SK, while other peripherals must be attached externally if needed.
// See https://aka.ms/AzureSphereHardwareDefinitions for more information on how to use hardware abstractions,
// to enable apps to work across multiple hardware variants.

// This file is autogenerated from ../../sample_appliance.json.  Do not edit it directly.

#pragma once
#include "avnet_mt3620_sk.h"

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
#define SAMPLE_LSM6DS3_SPI AVNET_MT3620_SK_ISU1_SPI

// MT3620 SPI Chip Select (CS) value "B". This is not a peripheral identifier, and so has no meaning in an app manifest.
#define SAMPLE_LSM6DS3_SPI_CS MT3620_SPI_CS_B

// MT3620 SK: Connect external NRF52 RESET signal using the PMOD connector: Pin-8.
#define SAMPLE_NRF52_RESET AVNET_MT3620_SK_GPIO17

// MT3620 SK: Connect external NRF52 DFU signal using the PMOD connector: Pin-7
#define SAMPLE_NRF52_DFU AVNET_MT3620_SK_GPIO2

// MT3620 SK: Connect external NRF52 UART using the PMOD connector): (RX Pin-3), (TX Pin-2), (CTS Pin-1), and (RTS Pin-4).
#define SAMPLE_NRF52_UART AVNET_MT3620_SK_ISU0_UART

// MT3620 SK: Connect external red LED using CLICK1, pin PWM.
#define SAMPLE_DEVICE_STATUS_LED AVNET_MT3620_SK_GPIO0

// MT3620 SK: Connect external blue LED using CLICK1, pin PWM2.
#define SAMPLE_PENDING_UPDATE_LED AVNET_MT3620_SK_APP_STATUS_LED_YELLOW

// MT3620 SK: LSM6DSO accelerometer.
#define SAMPLE_LSM6DSO_I2C AVNET_AESMS_ISU2_I2C

