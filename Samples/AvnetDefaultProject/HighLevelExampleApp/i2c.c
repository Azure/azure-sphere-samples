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

typedef union {
    int16_t i16bit[3];
    uint8_t u8bit[6];
} axis3bit16_t;

typedef union {
    int16_t i16bit;
    uint8_t u8bit[2];
} axis1bit16_t;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

typedef union {
    int32_t i32bit;
    uint8_t u8bit[4];
} axis1bit32_t;

static axis3bit16_t data_raw_acceleration;
static axis3bit16_t data_raw_angular_rate;
static axis3bit16_t raw_angular_rate_calibration;
AngularRateDegreesPerSecond angularRateDps;
AccelerationgForce accelerationgForce;
static uint8_t whoamI, rst;

// static uint8_t tx_buffer[1000];

int i2cFd = -1;
static stmdev_ctx_t dev_ctx;
static stmdev_ctx_t pressure_ctx;
static bool initialized = false;
bool lps22hhDetected = false;

// Global variables to hold the most recent sensor data
AccelerationgForce acceleration_g;
AngularRateDegreesPerSecond angular_rate_dps;
float lsm6dso_temperature;
float pressure_kPa;
float lps22hh_temperature;

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
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay(uint32_t ms);
static void platform_init(void);
static int32_t lsm6dso_read_lps22hh_cx(void *ctx, uint8_t reg, uint8_t *data, uint16_t len);
static int32_t lsm6dso_write_lps22hh_cx(void *ctx, uint8_t reg, uint8_t *data, uint16_t len);

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    uint8_t cmdBuffer[len + 1];
    cmdBuffer[0] = reg;
    memcpy(&cmdBuffer[1], bufp, (size_t)len);

    int32_t retVal = I2CMaster_Write(*(int *)handle, LSM6DSO_ADDRESS, cmdBuffer, (size_t)(len + 1));
    if (retVal != len + 1) {
        Log_Debug("ERROR: Expected return value to match count\n");
    }

    return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    int32_t retVal =
        I2CMaster_WriteThenRead(*(int *)handle, LSM6DSO_ADDRESS, &reg, 1, bufp, (size_t)len);
    if (retVal < 0) {
        Log_Debug("ERROR: Expected return value to match count\n");
    }

    return 0;
}

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
	//oled_draw_logo();
	oled_i2c_bus_status(CLEAR_BUFFER);
#endif

}

AccelerationgForce lp_get_acceleration(void)
{
    uint8_t reg;

    if (!initialized) {
        accelerationgForce.x = accelerationgForce.y = accelerationgForce.z = NAN;
        return accelerationgForce;
    }

    /* Read output only if new xl value is available */
    lsm6dso_xl_flag_data_ready_get(&dev_ctx, &reg);
    if (reg) {
        /* Read acceleration field data */
        memset(data_raw_acceleration.u8bit, 0x00, 3 * sizeof(int16_t));
        lsm6dso_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);

        // Not sure which conversion fucntion to use?
        // https://github.com/STMicroelectronics/STMems_Standard_C_drivers/blob/master/lsm6dso_STdC/example/lsm6dso_sensor_hub_lps22hh.c

        // AccelerationgForce.x = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[0]);
        // AccelerationgForce.y = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[1]);
        // AccelerationgForce.z = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[2]);

        // Reads the acceleration and convert it from milig to g
        accelerationgForce.x = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0])/1000;
        accelerationgForce.y = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1])/1000;
        accelerationgForce.z = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2])/1000;

        // Log_Debug("x %f, y %f, z %f\n", AccelerationgForce.x, AccelerationgForce.y,
        // AccelerationgForce.z);
    }

    return accelerationgForce;
}

AngularRateDegreesPerSecond lp_get_angular_rate(void)
{
    uint8_t reg;

    if (!initialized) {
        angularRateDps.x = angularRateDps.y = angularRateDps.z = NAN;
        return angularRateDps;
    }

    lsm6dso_gy_flag_data_ready_get(&dev_ctx, &reg);
    if (reg) {
        /* Read angular rate field data */
        memset(data_raw_angular_rate.u8bit, 0x00, 3 * sizeof(int16_t));
        lsm6dso_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);

        angularRateDps.x = (lsm6dso_from_fs2000_to_mdps(data_raw_angular_rate.i16bit[0] -
                                                        raw_angular_rate_calibration.i16bit[0])) /
                           1000.0;
        angularRateDps.y = (lsm6dso_from_fs2000_to_mdps(data_raw_angular_rate.i16bit[1] -
                                                        raw_angular_rate_calibration.i16bit[1])) /
                           1000.0;
        angularRateDps.z = (lsm6dso_from_fs2000_to_mdps(data_raw_angular_rate.i16bit[2] -
                                                        raw_angular_rate_calibration.i16bit[2])) /
                           1000.0;

        // Log_Debug("x %f, y %f, z %f\n", angularRateDps.x, angularRateDps.y, angularRateDps.z);
    }

    return angularRateDps;
}

float lp_get_temperature_lps22h(void) // get_temperature() from lsm6dso is faster
{
    lps22hh_reg_t lps22hhReg;
    int16_t i16bit;
    static float lps22hhTemperature_degC = NAN;

    if (!initialized) {
        return NAN;
    }

    if (lps22hhDetected) {
        i16bit = 0;

        lps22hh_read_reg(&pressure_ctx, LPS22HH_STATUS, (uint8_t *)&lps22hhReg, 1);

        // Read output only if new value is available

        if ((lps22hhReg.status.p_da == 1) && (lps22hhReg.status.t_da == 1)) {
            lps22hh_temperature_raw_get(&pressure_ctx, &i16bit);
            lps22hhTemperature_degC = lps22hh_from_lsb_to_celsius(i16bit);
        }
        return lps22hhTemperature_degC;
    }
    return NAN;
}

float lp_get_temperature(void)
{
    uint8_t reg;
    axis1bit16_t data_raw_temperature;

    if (!initialized) {
        return NAN;
    }

    lsm6dso_temp_flag_data_ready_get(&dev_ctx, &reg);
    if (reg) {
        /* Read temperature data */
        memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
        lsm6dso_temperature_raw_get(&dev_ctx, data_raw_temperature.u8bit);
        return lsm6dso_from_lsb_to_celsius(data_raw_temperature.i16bit);
    }

    return NAN;
}

float lp_get_pressure(void)
{
    lps22hh_reg_t lps22hhReg;
    uint32_t ui32bit;
    static float pressure_kPa = NAN;

    if (!initialized) {
        return NAN;
    }

    if (lps22hhDetected) {
        ui32bit = 0;

        lps22hh_read_reg(&pressure_ctx, LPS22HH_STATUS, (uint8_t *)&lps22hhReg, 1);

        // Read output only if new value is available

        if ((lps22hhReg.status.p_da == 1) && (lps22hhReg.status.t_da == 1)) {
            lps22hh_pressure_raw_get(&pressure_ctx, &ui32bit);
            pressure_kPa = lps22hh_from_lsb_to_hpa(ui32bit)/1000;
        }
        return pressure_kPa;
    }
    return NAN;
}

void lp_calibrate_angular_rate(void)
{
    if (!initialized) {
        return;
    }

    // Read the raw angular rate data from the device to use as offsets.  We're making the
    // assumption that the device is stationary.

    uint8_t reg;

    Log_Debug("LSM6DSO: Calibrating angular rate . . .\n");
    Log_Debug("LSM6DSO: Please make sure the device is stationary.\n");

    do {

        // Delay and read the device until we have data!
        do {
            // Read the calibration values
            platform_delay(500);
            lsm6dso_gy_flag_data_ready_get(&dev_ctx, &reg);
        } while (!reg);

        if (reg) {
            // Read angular rate field data to use for calibration offsets
            memset(data_raw_angular_rate.u8bit, 0x00, 3 * sizeof(int16_t));
            lsm6dso_angular_rate_raw_get(&dev_ctx, raw_angular_rate_calibration.u8bit);
        }

        // Delay and read the device until we have data!
        do {
            // Read the calibration values
            platform_delay(500);
            lsm6dso_gy_flag_data_ready_get(&dev_ctx, &reg);
        } while (!reg);

        // Read the angular data rate again and verify that after applying the calibration, we have
        // 0 angular rate in all directions
        if (reg) {
            // Read angular rate field data
            memset(data_raw_angular_rate.u8bit, 0x00, 3 * sizeof(int16_t));
            lsm6dso_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);

            // Before we store the mdps values subtract the calibration data we captured at startup.
            angularRateDps.x = lsm6dso_from_fs2000_to_mdps((int16_t)(
                data_raw_angular_rate.i16bit[0] - (int)raw_angular_rate_calibration.i16bit[0]));
            angularRateDps.y = lsm6dso_from_fs2000_to_mdps((int16_t)(
                data_raw_angular_rate.i16bit[1] - raw_angular_rate_calibration.i16bit[1]));
            angularRateDps.z = lsm6dso_from_fs2000_to_mdps((int16_t)(
                data_raw_angular_rate.i16bit[2] - raw_angular_rate_calibration.i16bit[2]));
        }

        // If the angular values after applying the offset are not all 0.0s, then do it again!
    } while ((angularRateDps.x != 0.0) || (angularRateDps.y != 0.0) || (angularRateDps.z != 0.0));

    Log_Debug("LSM6DSO: Calibrating angular rate complete!\n");
}

bool detect_lps22hh(void)
{
    int failCount = 10;

    while (!lps22hhDetected) {

        // Enable pull up on master I2C interface.
        lsm6dso_sh_pin_mode_set(&dev_ctx, LSM6DSO_INTERNAL_PULL_UP);

        // Check if LPS22HH is connected to Sensor Hub
        lps22hh_device_id_get(&pressure_ctx, &whoamI);
        if (whoamI != LPS22HH_ID) {
            Log_Debug("LPS22HH not found!\n");
        } else {
            lps22hhDetected = true;
            Log_Debug("LPS22HH Found!\n");
        }

        // Restore the default configuration
        lps22hh_reset_set(&pressure_ctx, PROPERTY_ENABLE);
        do {
            lps22hh_reset_get(&pressure_ctx, &rst);
        } while (rst);

        // Enable Block Data Update
        lps22hh_block_data_update_set(&pressure_ctx, PROPERTY_ENABLE);

        // Set Output Data Rate
        lps22hh_data_rate_set(&pressure_ctx, LPS22HH_10_Hz_LOW_NOISE);

        // If we failed to detect the lps22hh device, then pause before trying again.
        if (!lps22hhDetected) {
            platform_delay(100);
        }

        if (failCount-- == 0) {
            lps22hhDetected = false;
            Log_Debug(
                "Failed to read LPS22HH device ID, disabling all access to LPS22HH device!\n");
            Log_Debug("Usually a power cycle will correct this issue\n");
            break;
        }
    }

    return lps22hhDetected;
}

void lp_imu_initialize(void)
{
    if (initialized) {
        return;
    }

    /* Initialize mems driver interface */
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.handle = &i2cFd;

    // Initialize lps22hh mems driver interface
    pressure_ctx.read_reg = lsm6dso_read_lps22hh_cx;
    pressure_ctx.write_reg = lsm6dso_write_lps22hh_cx;
    pressure_ctx.handle = &i2cFd;

    /* Init test platform */
    platform_init();

    /* Wait sensor boot time */
    platform_delay(20);

    /* Check device ID */
    lsm6dso_device_id_get(&dev_ctx, &whoamI);
    if (whoamI != LSM6DSO_ID) {
        initialized = false;
        Log_Debug("LSM6DS0 not Found!\n");
#ifdef OLED_SD1306
		// OLED update
		lsm6dso_status = 1;
		oled_i2c_bus_status(LSM6DSO_STATUS_DISPLAY);
#endif
        return;
    }
    Log_Debug("LSM6DS0 Found!\n");
#ifdef OLED_SD1306
		// OLED update
		lsm6dso_status = 0;
		oled_i2c_bus_status(LSM6DSO_STATUS_DISPLAY);
#endif

    /* Restore default configuration */
    lsm6dso_reset_set(&dev_ctx, PROPERTY_ENABLE);
    do {
        lsm6dso_reset_get(&dev_ctx, &rst);
    } while (rst);

    /* Disable I3C interface */
    lsm6dso_i3c_disable_set(&dev_ctx, LSM6DSO_I3C_DISABLE);

    /* Enable Block Data Update */
    lsm6dso_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

    /* Set Output Data Rate */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_12Hz5);
    lsm6dso_gy_data_rate_set(&dev_ctx, LSM6DSO_GY_ODR_12Hz5);

    /* Set full scale */
    lsm6dso_xl_full_scale_set(&dev_ctx, LSM6DSO_2g);
    lsm6dso_gy_full_scale_set(&dev_ctx, LSM6DSO_2000dps);

    /* Configure filtering chain(No aux interface)
     * Accelerometer - LPF1 + LPF2 path
     */
    lsm6dso_xl_hp_path_on_out_set(&dev_ctx, LSM6DSO_LP_ODR_DIV_100);
    lsm6dso_xl_filter_lp2_set(&dev_ctx, PROPERTY_ENABLE);

    if (detect_lps22hh()) {
#ifdef OLED_SD1306	    
        lps22hh_status = 1;
#endif         
        initialized = true;
    }

#ifdef OLED_SD1306
    // OLED update
	lps22hh_status = 0;
	oled_i2c_bus_status(LPS22HH_STATUS);
#endif 

    lp_calibrate_angular_rate();


    // read_imu();

    /* Read samples in polling mode (no int) */
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

/*
 * @brief  Write lsm2mdl device register (used by configuration functions)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t lsm6dso_write_lps22hh_cx(void *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
    axis3bit16_t data_raw_acceleration;
    int32_t ret;
    uint8_t drdy;
    lsm6dso_status_master_t master_status;
    lsm6dso_sh_cfg_write_t sh_cfg_write;

    // Configure Sensor Hub to write to the LPS22HH, and send the write data
    sh_cfg_write.slv0_add = (LPS22HH_I2C_ADD_L & 0xFEU) >> 1; // 7bit I2C address
    sh_cfg_write.slv0_subadd = reg, sh_cfg_write.slv0_data = *data,
    ret = lsm6dso_sh_cfg_write(&dev_ctx, &sh_cfg_write);

    /* Disable accelerometer. */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

    /* Enable I2C Master. */
    lsm6dso_sh_master_set(&dev_ctx, PROPERTY_ENABLE);

    /* Enable accelerometer to trigger Sensor Hub operation. */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

    /* Wait Sensor Hub operation flag set. */
    lsm6dso_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
    do {
        platform_delay(20);
        lsm6dso_xl_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    do {
        platform_delay(20);
        lsm6dso_sh_status_get(&dev_ctx, &master_status);
    } while (!master_status.sens_hub_endop);

    /* Disable I2C master and XL (trigger). */
    lsm6dso_sh_master_set(&dev_ctx, PROPERTY_DISABLE);
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

    return ret;
}

/*
 * @brief  Read lsm2mdl device register (used by configuration functions)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t lsm6dso_read_lps22hh_cx(void *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
    lsm6dso_sh_cfg_read_t sh_cfg_read;
    uint8_t buf_raw[6];
    int32_t ret;
    uint8_t drdy;
    lsm6dso_status_master_t master_status;

    /* Disable accelerometer. */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

    /* Configure Sensor Hub to read LPS22HH. */
    sh_cfg_read.slv_add = (LPS22HH_I2C_ADD_L & 0xFEU) >> 1; /* 7bit I2C address */
    sh_cfg_read.slv_subadd = reg;
    sh_cfg_read.slv_len = (uint8_t)len;

    // Call the command to read the data from the sensor hub.
    // This data will be read from the device connected to the
    // sensor hub, and saved into a register for us to read.
    ret = lsm6dso_sh_slv0_cfg_read(&dev_ctx, &sh_cfg_read);

    // Using slave 0 only
    lsm6dso_sh_slave_connected_set(&dev_ctx, LSM6DSO_SLV_0);

    /* Enable I2C Master */
    lsm6dso_sh_master_set(&dev_ctx, PROPERTY_ENABLE);

    /* Enable accelerometer to trigger Sensor Hub operation. */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

    /* Wait Sensor Hub operation flag set. */
    lsm6dso_acceleration_raw_get(&dev_ctx, buf_raw);
    do {
        platform_delay(20);
        lsm6dso_xl_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    do {
        platform_delay(20);
        lsm6dso_sh_status_get(&dev_ctx, &master_status);
    } while (!master_status.sens_hub_endop);

    /* Disable I2C master and XL(trigger). */
    lsm6dso_sh_master_set(&dev_ctx, PROPERTY_DISABLE);
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

    // Read the data from the device
    lsm6dso_sh_read_data_raw_get(&dev_ctx, data, (uint8_t)len);

#ifdef ENABLE_READ_WRITE_DEBUG
    Log_Debug("Read %d bytes: ", len);
    for (int i = 0; i < len; i++) {
        Log_Debug("[%0x] ", data[i]);
    }
    Log_Debug("\n", len);
#endif

    /* Re-enable accelerometer */
    lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

    return ret;
}