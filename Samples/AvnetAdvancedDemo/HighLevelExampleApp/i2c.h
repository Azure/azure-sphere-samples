#pragma once

#include "lsm6dso_reg.h"
#include "lps22hh_reg.h"
#include <applibs/i2c.h>
#include <applibs/log.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <hw/sample_appliance.h>
#include "oled.h"
#include "build_options.h"

#define LSM6DSO_ADDRESS 0x6A // I2C Address

typedef struct {
    float x;
    float y;
    float z;
} AngularRateDegreesPerSecond;

typedef struct {
    float x;
    float y;
    float z;
} AccelerationgForce;

extern bool lps22hhDetected;
extern AccelerationgForce acceleration_g;
extern AngularRateDegreesPerSecond angular_rate_dps;
extern float lsm6dso_temperature;
extern float pressure_kPa;
extern float lps22hh_temperature;
extern int i2cFd;


void lp_imu_initialize(void);
void lp_imu_close(void);
float lp_get_temperature(void);
float lp_get_pressure(void);
float lp_get_temperature_lps22h(void); // get_temperature() from lsm6dso is faster
void lp_calibrate_angular_rate(void);
AngularRateDegreesPerSecond lp_get_angular_rate(void);
AccelerationgForce lp_get_acceleration(void);
