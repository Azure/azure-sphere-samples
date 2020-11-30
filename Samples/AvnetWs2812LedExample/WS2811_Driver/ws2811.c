#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <applibs/gpio.h>
#include <applibs/log.h>
#include "../applibs_versions.h"
#include <applibs/spi.h>

#include "ws2811.h"

/* Using 2400000 bps (bits per second) */
/* Bit Lapse = 1 sec / 2400000 = 0.000000417 sec = 417ns */
/* Reset time = 4 integers * 32 bits / integer * 417ns / bit = 53376ns = 53.376 microseconds */

#define RESL 4  

static int pixelCount = -1;

static int spiFd = -1;

WS11_Pixel* pixels;

uint32_t zero[RESL];

void WS11_Color_SetValue(uint8_t value, WS11_Color *wscolor)
{
	wscolor->b1 = 0b1000001000001000;
	wscolor->b2 = 0b0010000010000010;
	wscolor->b3 = 0b0000100000100000;
	if (value == 0) return;
	if (0b10000000 & value) wscolor->b1 += 0b0110000000000000;
	if (0b01000000 & value) wscolor->b1 += 0b0000000110000000;
	if (0b00100000 & value) wscolor->b1 += 0b0000000000000110;
	if (0b00010000 & value) wscolor->b2 += 0b0001100000000000;
	if (0b00001000 & value) wscolor->b2 += 0b0000000001100000;
	if (0b00000100 & value)
	{
		wscolor->b2++;
		wscolor->b3 += 0b1000000000000000;
	}
	if (0b00000010 & value) wscolor->b3 += 0b0000011000000000;
	if (0b00000001 & value) wscolor->b3 += 0b0000000000011000;
}

void WS11_PixelStrip_SetColor(int index, uint8_t red, uint8_t green, uint8_t blue)
{
	if (index < 0)
	{
		for (int i = 0; i < pixelCount; i++)
		{
			WS11_Color_SetValue(red, &pixels[i].Red);
			WS11_Color_SetValue(green, &pixels[i].Green);
			WS11_Color_SetValue(blue, &pixels[i].Blue);
		}
		return;
	}

	if(index < pixelCount)
	{
		WS11_Color_SetValue(red, &pixels[index].Red);
		WS11_Color_SetValue(green, &pixels[index].Green);
		WS11_Color_SetValue(blue, &pixels[index].Blue);
	}
}


int  WS11_SPI_init(int spi)
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

	int result = SPIMaster_SetBusSpeed(spiFd, 2400000);  // 2400000 bps
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

int WS11_PixelStrip_Init(int count, int spi)
{
	/* Initialize SPI */
	if(WS11_SPI_init(spi) < 0) return -1;

	/* Initialize data to reset signal */
	for (int i = 0; i < RESL; i++)
		zero[i] = 0;

	pixels = (WS11_Pixel*)calloc(count, sizeof(WS11_Pixel));
	if (pixels == 0) return -1;	

	pixelCount = count;
	WS11_PixelStrip_SetColor(-1, 0, 0, 0);

	return 1;
}

void WS11_PixelStrip_Show()
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
	trans.length = 18 * pixelCount;
	res = SPIMaster_TransferSequential(spiFd, &trans, 1);
}