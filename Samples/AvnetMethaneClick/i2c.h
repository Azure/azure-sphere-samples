#pragma once

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

extern int i2cFd;

void lp_imu_initialize(void);
void lp_imu_close(void);
