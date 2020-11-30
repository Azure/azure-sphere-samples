#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <applibs/gpio.h>
#include <applibs/log.h>
#include "../applibs_versions.h"
#include <applibs/spi.h>
#include "ws2812b.h"

/* Using 3400000 bps (bits per second) */

/* Bit Lapse */
// 1 sec / 2400000 = 0.000000294 sec = 294ns */

/* Reset Time */
// 6 integers * 32 bits / integer * 294ns / bit = 56448ns = 56.448 microseconds

/* Some strips need around 400ns for bit lapse, use 2400000bps, and 4 integers for reset time */

#define RESL 6

static int pixelCount = -1;

static int spiFd = -1;

WS_Pixel* pixels;

uint32_t zero[RESL];

void WS_Color_SetValue(uint8_t value, WS_Color *wscolor)
{
	wscolor->b1 = 0b10010010;
	wscolor->b2 = 0b01001001;
	wscolor->b3 = 0b00100100;
	if (value == 0) return;
	if (0b10000000 & value) wscolor->b1 += 0b01000000;
	if (0b01000000 & value) wscolor->b1 += 0b00001000;
	if (0b00100000 & value) wscolor->b1 += 0b00000001;
	if (0b00010000 & value) wscolor->b2 += 0b00100000;
	if (0b00001000 & value) wscolor->b2 += 0b00000100;
	if (0b00000100 & value) wscolor->b3 += 0b10000000;
	if (0b00000010 & value) wscolor->b3 += 0b00010000;
	if (0b00000001 & value) wscolor->b3 += 0b00000010;
}

void WS_PixelStrip_SetColor(int index, uint8_t red, uint8_t green, uint8_t blue)
{
	if (index < 0)
	{
		for (int i = 0; i < pixelCount; i++)
		{
			WS_Color_SetValue(red, &pixels[i].Red);
			WS_Color_SetValue(green, &pixels[i].Green);
			WS_Color_SetValue(blue, &pixels[i].Blue);
		}
		return;
	}

	if(index < pixelCount)
	{
		WS_Color_SetValue(red, &pixels[index].Red);
		WS_Color_SetValue(green, &pixels[index].Green);
		WS_Color_SetValue(blue, &pixels[index].Blue);
	}
}


int  SPI_init(int spi)
{
	SPIMaster_Config config;
	int ret = SPIMaster_InitConfig(&config);
	if (ret != 0) {
		Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
			errno);
		return -1;
	}
	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	spiFd = SPIMaster_Open(spi, -1, &config);
	if (spiFd < 0) {
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = SPIMaster_SetBusSpeed(spiFd, 3400000);  // Adjust bps if needed
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_1);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetBitOrder(spiFd, SPI_BitOrder_MsbFirst);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetBitOrder: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

}

int WS_PixelStrip_Init(int count, int spi)
{
	/* Initialize SPI */
	if(SPI_init(spi) < 0) return -1;

	/* Initialize data to reset signal */
	for (int i = 0; i < RESL; i++)
		zero[i] = 0;

	pixels = (WS_Pixel*)calloc(count, sizeof(WS_Pixel));
	if (pixels == 0) return -1;	

	pixelCount = count;
	WS_PixelStrip_SetColor(-1, 0, 0, 0);

	return 1;
}

void WS_PixelStrip_Show()
{
	/* Reset the signal */
	SPIMaster_Transfer trans;
	SPIMaster_InitTransfers(&trans, 1);
	trans.flags = SPI_TransferFlags_Write;
	trans.writeData = zero;
	trans.length = RESL * 4;
	int res = SPIMaster_TransferSequential(spiFd, &trans, 1);


	/* Send new signal */
	trans.writeData = pixels;
	trans.length = 9 * pixelCount;
	res = SPIMaster_TransferSequential(spiFd, &trans, 1);
}