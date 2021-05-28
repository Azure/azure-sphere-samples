/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "i2c.h"

#ifndef GUARDIAN_100

int i2cFd = -1;
static bool initialized = false;

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

void lp_imu_initialize(void)
{
    /* Init test platform */
    platform_init();

    /* Wait sensor boot time */
    platform_delay(20);

    initialized = true;
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

#endif // !GUARDIAN_100