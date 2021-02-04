/*
 ******************************************************************************
 * @file    read_data_simple.c
 * @author  Sensors Software Solution Team
 * @brief   This file show the simplest way to get data from sensor.
 *
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

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 + STEVAL-MKI196V1
 * - NUCLEO_F411RE + X_NUCLEO_IKS01A3
 *
 * and STM32CubeMX tool with STM32CubeF4 MCU Package
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */
//#define STEVAL_MKI109V3

/*
Driver ported by: Dave Glover
Date: August 2020
Acknowledgment: Built from ST Micro samples
        https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lsm6dso_STdC
        https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lps22hh_STdC
        https://github.com/STMicroelectronics/STMems_Standard_C_drivers/blob/master/lsm6dso_STdC/example/lsm6dso_sensor_hub_lps22hh.c

*/

#include "i2c.h"

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


// static uint8_t tx_buffer[1000];

int i2cFd = -1;

static bool initialized = false;

// Global variables to hold the most recent sensor data

#ifdef OLED_SD1306
// Status variables
uint8_t lsm6dso_status = 1;
uint8_t lps22hh_status = 1;
uint8_t RTCore_status = 1;
#endif 

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static void platform_delay(uint32_t ms);
static void platform_init(void);

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
    struct timespec ts;

    ts.tv_sec = (long int)(ms / 1000u);
    ts.tv_nsec = (long int)((ms - ((long unsigned int)ts.tv_sec * 1000u)) * 1000000u);

    nanosleep(&ts, NULL);
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
    i2cFd = I2CMaster_Open(AVNET_MT3620_SK_ISU2_I2C);
    if (i2cFd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return;
    }

    int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return;
    }

    result = I2CMaster_SetTimeout(i2cFd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return;
    }

#ifdef OLED_SD1306
	// Start OLED
	if (oled_init())
	{
		Log_Debug("OLED not found!\n");
	}
	else
	{
		Log_Debug("OLED found!\n");
	}

	// Draw AVNET logo
	oled_draw_logo();
#endif

}


void lp_imu_initialize(void)
{
    if (initialized) {
        return;
    }

    /* Init test platform */
    platform_init();

    /* Wait sensor boot time */
    platform_delay(20);

#ifdef OLED_SD1306
	// OLED update
    oled_draw_logo();
#endif
       return;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdPrintError(int fd, const char *fdName)
{
    if (initialized && fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
    initialized = false;
}

/// <summary>
///     Closes the I2C interface File Descriptors.
/// </summary>
void lp_imu_close(void)
{
    CloseFdPrintError(i2cFd, "i2c");
}