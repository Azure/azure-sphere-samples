#if defined(AzureSphere_CA7)

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/i2c.h>
#include <applibs/spi.h>
#include <applibs/gpio.h>
#include "hw/sample_appliance.h"

static int GpioFd;
static int i2cFd;
static int spiFd;

#define MAX_SPI_TRANSFER_BYTES	4096

#elif defined(AzureSphere_CM4)

#include <stdlib.h>
#include <string.h>

#include "SPIMaster.h"
#include "I2CMaster.h"
#include "GPIO.h"
#include "Log_Debug.h"

#define MT3620_RDB_HEADER2_ISU0_I2C		MT3620_UNIT_ISU0
#define MT3620_RDB_HEADER4_ISU1_SPI		MT3620_UNIT_ISU1
#define MT3620_RDB_HEADER1_PIN10_GPIO	3

static SPIMaster* SpiHandler;
static I2CMaster* I2cHandler;

#define MAX_SPI_TRANSFER_BYTES	16

#endif

#include "ll.h"




#ifdef USE_OV2640
#define OV2640_I2C_ADDR 0x30
int sensor_addr = OV2640_I2C_ADDR;
#endif 
#ifdef USE_OV5642
#define OV5642_I2C_ADDR 0x3C
int sensor_addr = OV5642_I2C_ADDR;
#endif 

#if defined(USE_OV2640) & defined(USE_OV5642)
#error Only one camera type can be defined
#endif 

int ll_gpio_init(void)
{
#if defined(AzureSphere_CA7)

	GpioFd = GPIO_OpenAsOutput(ARDUCAM_CS, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (GpioFd < 0) {
		Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = GPIO_ConfigurePinForOutput(MT3620_RDB_HEADER1_PIN10_GPIO);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: GPIO_ConfigurePinForOutput: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

void ll_gpio_cs_go_low(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(GpioFd, GPIO_Value_Low);

#elif defined(AzureSphere_CM4)
	
	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN10_GPIO, 0);

#endif
}

void ll_gpio_cs_go_high(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(GpioFd, GPIO_Value_High);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN10_GPIO, 1);

#endif
}

int ll_i2c_init(void)
{
#if defined(AzureSphere_CA7)

    i2cFd = I2CMaster_Open(ARDUCAM_I2C);
	if (i2cFd < 0) {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	int ret = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (ret < 0) {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\r\n", errno, strerror(errno));
		close(i2cFd);
		return -1;
	}

	ret = I2CMaster_SetTimeout(i2cFd, 100);
	if (ret < 0) {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\r\n", errno, strerror(errno));
		close(i2cFd);
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	I2cHandler = I2CMaster_Open(MT3620_UNIT_ISU0);
	I2CMaster_SetBusSpeed(I2cHandler, I2C_BUS_SPEED_STANDARD);

	return 0;

#endif
}

int ll_i2c_tx(uint8_t* tx_data, uint32_t tx_len)
{
#if defined(AzureSphere_CA7)

	int ret = I2CMaster_Write(i2cFd, sensor_addr, tx_data, tx_len);
	if (ret < 0) {
		Log_Debug("ERROR: I2CMaster_Write: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	} else if (ret != tx_len) {
		Log_Debug("ERROR: I2CMaster_Write transfer %d bytes, expect %d bytes\r\n", ret, tx_len);
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = I2CMaster_WriteSync(I2cHandler, OV2640_I2C_ADDR, tx_data, tx_len);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: I2CMaster_WriteSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ll_i2c_tx_then_rx(uint8_t* tx_data, uint32_t tx_len, uint8_t* rx_data, uint32_t rx_len)
{
#if defined(AzureSphere_CA7)

	int ret = I2CMaster_WriteThenRead(i2cFd, sensor_addr, tx_data, tx_len, rx_data, rx_len);
	if (ret < 0) {
		Log_Debug("ERROR: I2CMaster_WriteThenRead: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	} else if (ret != (tx_len + rx_len)) {
		Log_Debug("ERROR: I2CMaster_WriteThenRead transfer %d bytes, expect %d bytes\r\n", ret, tx_len + rx_len);
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = I2CMaster_WriteThenReadSync(I2cHandler, OV2640_I2C_ADDR, tx_data, tx_len, rx_data, rx_len);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: I2CMaster_WriteThenReadSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ll_spi_init(void)
{
#if defined(AzureSphere_CA7)

    SPIMaster_Config config;
    int ret = SPIMaster_InitConfig(&config);
    if (ret < 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig: errno=%d (%s)\r\n", errno, strerror(errno));
        return -1;
    }

    config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    spiFd = SPIMaster_Open(ARDUCAM_SPI, MT3620_SPI_CS_A, &config);
    if (spiFd < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\r\n", errno, strerror(errno));
        return -1;
    }

    int result = SPIMaster_SetBusSpeed(spiFd, 8000000);
    if (result < 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\r\n", errno, strerror(errno));
        close(spiFd);
        return -1;
    }

    result = SPIMaster_SetMode(spiFd, SPI_Mode_0);
    if (result < 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\r\n", errno, strerror(errno));
        close(spiFd);
        return -1;
    }
	
	return 0;

#elif defined(AzureSphere_CM4)

	SpiHandler = SPIMaster_Open(MT3620_UNIT_ISU1);
	SPIMaster_DMAEnable(SpiHandler, false);
	SPIMaster_Configure(SpiHandler, false, false, 4000000);

	return 0;

#endif
}

int ll_spi_tx(uint8_t *tx_data, uint32_t tx_len)
{
#if defined(AzureSphere_CA7)

	if (tx_len > MAX_SPI_TRANSFER_BYTES) {
		Log_Debug("ll_spi_tx does not support split transfer when data len > 4096\r\n");
		return -1;
	}

	SPIMaster_Transfer transfers;

	int ret = SPIMaster_InitTransfers(&transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	transfers.flags     = SPI_TransferFlags_Write;
	transfers.writeData = tx_data;
	transfers.length    = tx_len;

	ret = SPIMaster_TransferSequential(spiFd, &transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	} else if (ret != tx_len) {
		Log_Debug("ERROR: SPIMaster_TransferSequential transfer %d bytes, expect %d bytes\r\n", ret, tx_len);
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = SPIMaster_WriteSync(SpiHandler, tx_data, tx_len);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SPIMaster_WriteSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ll_spi_rx(uint8_t *rx_data, uint32_t rx_len)
{
#if defined(AzureSphere_CA7)

	size_t numOfXfer = (rx_len % MAX_SPI_TRANSFER_BYTES == 0) ? (rx_len / MAX_SPI_TRANSFER_BYTES) : (rx_len / MAX_SPI_TRANSFER_BYTES + 1);
	
	SPIMaster_Transfer transfer;

	uint32_t offset = 0;
	int32_t sizeleft = (int32_t)rx_len;
	
	while (numOfXfer > 0) {
		int ret = SPIMaster_InitTransfers(&transfer, 1);
		if (ret < 0) {
			Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
			return -1;
		}

		transfer.flags = SPI_TransferFlags_Read;
		transfer.readData = rx_data + offset;
		transfer.length = (sizeleft > MAX_SPI_TRANSFER_BYTES) ? MAX_SPI_TRANSFER_BYTES : sizeleft;

		ret = SPIMaster_TransferSequential(spiFd, &transfer, 1);
		if (ret < 0) {
			Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
			return -1;
		} else if (ret != transfer.length) {
			Log_Debug("ERROR: SPIMaster_TransferSequential transfer %d bytes, expect %d bytes\r\n", ret, rx_len);
			return -1;
		}

		sizeleft -= MAX_SPI_TRANSFER_BYTES;
		offset   += MAX_SPI_TRANSFER_BYTES;
		numOfXfer--;
	};
	
    return 0;

#elif defined(AzureSphere_CM4)
#if 0
	int32_t ret = SPIMaster_ReadSync(SpiHandler, rx_data, rx_len);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SPIMaster_ReadSync: %d\r\n", ret);
		return -1;
	}

	return 0;
#endif
	uint32_t numOfXfer = (rx_len % MAX_SPI_TRANSFER_BYTES == 0) ? (rx_len / MAX_SPI_TRANSFER_BYTES) : (rx_len / MAX_SPI_TRANSFER_BYTES + 1);

	uint32_t offset = 0;
	int32_t sizeleft = (int32_t)rx_len;

	while (numOfXfer > 0) {

		int32_t ret = SPIMaster_ReadSync(SpiHandler, rx_data + offset, sizeleft > MAX_SPI_TRANSFER_BYTES ? MAX_SPI_TRANSFER_BYTES : sizeleft);
		if (ret != ERROR_NONE) {
			Log_Debug("ERROR: SPIMaster_ReadSync: %d\r\n", ret);
			return -1;
		}

		sizeleft -= MAX_SPI_TRANSFER_BYTES;
		offset += MAX_SPI_TRANSFER_BYTES;
		numOfXfer--;
	};

	return 0;

#endif
}

int ll_spi_tx_then_rx(uint8_t *tx_data, uint32_t tx_len, uint8_t *rx_data, uint32_t rx_len)
{
#if defined(AzureSphere_CA7)
#if 0
	if ((tx_len > MAX_SPI_TRANSFER_BYTES) || (rx_len > MAX_SPI_TRANSFER_BYTES)) {
		Log_Debug("ll_spi_tx_then_rx does not support split transfer when data len > 4096\r\n");
		return -1;
	}
#endif
    int ret;
    ret = SPIMaster_WriteThenRead(spiFd, (const uint8_t *)tx_data, tx_len, rx_data, rx_len);
    if (ret < 0) {
        Log_Debug("ERROR: SPIMaster_WriteThenRead: errno=%d (%s)\r\n", errno, strerror(errno));
        return -1;
	} else if (ret != (tx_len + rx_len)) {
		Log_Debug("ERROR: SPIMaster_TransferSequential transfer %d bytes, expect %d bytes\r\n", ret, tx_len);
		return -1;
	}
    return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = SPIMaster_WriteThenReadSync(SpiHandler, tx_data, tx_len, rx_data, rx_len);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SPIMaster_WriteThenReadSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}
