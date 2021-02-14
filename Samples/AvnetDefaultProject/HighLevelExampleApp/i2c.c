#include "i2c.h"

int i2cFd = -1;
static bool initialized = false;

#ifdef OLED_SD1306
// Status variables
uint8_t RTCore_status = 1;
#endif 

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
