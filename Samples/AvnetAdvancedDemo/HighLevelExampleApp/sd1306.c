/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Rickey Castillo

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
#include "sd1306.h"
#include "font.h"

// pixel data of OLED screen
uint8_t oled_buffer[BUFFER_SIZE];

// Lock up table to reverse byte's bits 
static const uint8_t BitReverseTable256[] =
{
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/**
  * @brief  Send command to sd1306.
  * @param  addr: address of device
  * @param  cmd: commandto send
  * @retval retval: negative if was unsuccefully, positive if was succefully
  */
int32_t sd1306_send_command(uint8_t addr, uint8_t cmd)
{
	int32_t retval;
	uint8_t data_to_send[2];
	// Byte to tell sd1306 to process byte as command
	data_to_send[0] = 0x00;
	// Commando to send
	data_to_send[1] = cmd;
	// Send the data by I2C bus
	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 2);
	return retval;
}

/**
  * @brief  Send data to sd1306 RAM.
  * @param  addr: address of device
  * @param  data: pointer to data
  * @retval retval: negative if was unsuccefully, positive if was succefully
  */
int32_t sd1306_write_data(uint8_t addr, uint8_t *data)
{
	int32_t retval;
	uint16_t i;
	uint8_t data_to_send[1025];
	// Byte to tell sd1306 to process byte as data
	data_to_send[0] = 0x40;

	// Copy data to buffer
	for (i = 0; i < 1024; i++)
	{
		data_to_send[i + 1] = data[i];
	}

	// Send the data by I2C bus
	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 1025);
	return retval;
}

/**
  * @brief  Initialize sd1306.
  * @param  None.
  * @retval Positive if was unsuccefully, zero if was succefully.
  */
uint8_t sd1306_init(void)
{
	// OLED turn off and check if OLED is connected
	if (sd1306_send_command(sd1306_ADDR, 0xae) < 0)
	{
		return 1;
	}
	// Set display oscillator freqeuncy and divide ratio
	sd1306_send_command(sd1306_ADDR, 0xd5);
	sd1306_send_command(sd1306_ADDR, 0x50);

	// Set multiplex ratio
	sd1306_send_command(sd1306_ADDR, 0xa8);
	sd1306_send_command(sd1306_ADDR, 0x3f);
	// Set display start line
	sd1306_send_command(sd1306_ADDR, 0xd3);
	sd1306_send_command(sd1306_ADDR, 0x00);
	// Set the lower comulmn address
	sd1306_send_command(sd1306_ADDR, 0x00);
	// Set the higher comulmn address
	sd1306_send_command(sd1306_ADDR, 0x10);
	
	// Set page address
	sd1306_send_command(sd1306_ADDR, 0xb0);

	// Charge pump
	sd1306_send_command(sd1306_ADDR, 0x8d);
	sd1306_send_command(sd1306_ADDR, 0x14);
	
	// Memory mode
	sd1306_send_command(sd1306_ADDR, 0x20);
	sd1306_send_command(sd1306_ADDR, 0x00);

	// Set segment from left to right
	sd1306_send_command(sd1306_ADDR, 0xa0 | 0x01);
	// Set OLED upside up
	sd1306_send_command(sd1306_ADDR, 0xc8);
	// Set common signal pad configuration
	sd1306_send_command(sd1306_ADDR, 0xda);
	sd1306_send_command(sd1306_ADDR, 0x12);

	// Set Contrast
	sd1306_send_command(sd1306_ADDR, 0x81);
	// Contrast data
	sd1306_send_command(sd1306_ADDR, 0x00);

	// Set discharge precharge periods
	sd1306_send_command(sd1306_ADDR, 0xd9);
	sd1306_send_command(sd1306_ADDR, 0xf1);

	// Set common mode pad output voltage 
	sd1306_send_command(sd1306_ADDR, 0xdb);
	sd1306_send_command(sd1306_ADDR, 0x40);

	// Set Enire display
	sd1306_send_command(sd1306_ADDR, 0xa4);
	
	// Set Normal display
	sd1306_send_command(sd1306_ADDR, 0xa6);
	// Stop scroll
	sd1306_send_command(sd1306_ADDR, 0x2e);

	// OLED turn on
	sd1306_send_command(sd1306_ADDR, 0xaf);

	// Set column address
	sd1306_send_command(sd1306_ADDR, 0x21);
	// Start Column
	sd1306_send_command(sd1306_ADDR, 0x00);
	// Last column
	sd1306_send_command(sd1306_ADDR, 127);

	// Set page address
	sd1306_send_command(sd1306_ADDR, 0x22);
	// Start Page
	sd1306_send_command(sd1306_ADDR, 0x00);
	// Last Page
	sd1306_send_command(sd1306_ADDR, 0x07);

	return 0;
}

/**
  * @brief  Draw a pixel at specified coordinates
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @retval None.
  */
void sd1306_draw_pixel(int32_t x, int32_t y, uint8_t color)
{
	// Verify that pixel is inside of OLED matrix
	if (x >= 0 && x < 128 && y >= 0 && y < 64)
	{
		switch (color)
		{
			case 0:
			{
				oled_buffer[x + (y / 8) * 128] &= ~(1 << (y & 7));
			}
			break;
			case 1:
			{
				oled_buffer[x + (y / 8) * 128] |= (1 << (y & 7));
			}
			break;
			case 2:
			{
				oled_buffer[x + (y / 8) * 128] ^= (1 << (y & 7));
			}
			break;
			default:
			break;
		}
	}
}

/**
  * @brief  Draw a line
  * @param  x1: x coordinate of start point
  * @param  y1: y coordinate of start point
  * @param  x2: x coordinate of end point
  * @param  y2: y coordinate of end point
  * @retval None.
  */
void sd1306_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
	int32_t x;
	int32_t y;
	int32_t addx;
	int32_t addy;
	int32_t dx;
	int32_t dy;
	int32_t P;
	int32_t i;

	dx = abs((int32_t)(x2 - x1));
	dy = abs((int32_t)(y2 - y1));
	x = x1;
	y = y1;

	if (x1 > x2)
	{
		addx = -1;
	}
	else
	{
		addx = 1;
	}
	if (y1 > y2)
	{
		addy = -1;
	}
	else
	{
		addy = 1;
	}

	if (dx >= dy)
	{
		P = 2 * dy - dx;

		for (i = 0; i <= dx; ++i)
		{
			sd1306_draw_pixel(x, y, color);

			if (P < 0)
			{
				P += 2 * dy;
				x += addx;
			}
			else
			{
				P += 2 * dy - 2 * dx;
				x += addx;
				y += addy;
			}
		}
	}
	else
	{
		P = 2 * dx - dy;

		for (i = 0; i <= dy; ++i)
		{
			sd1306_draw_pixel(x, y, color);

			if (P < 0)
			{
				P += 2 * dx;
				y += addy;
			}
			else
			{
				P += 2 * dx - 2 * dy;
				x += addx;
				y += addy;
			}
		}
	}
}

/**
  * @brief  Draw a vertical line
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  length: length of the line
  * @retval None.
  */
void sd1306_draw_fast_Vline(uint8_t x, uint8_t y, uint8_t length, uint8_t color)
{
	while (length > 0)
	{
		sd1306_draw_pixel(x, y, color);
		y++;
		length--;
	}
}

/**
  * @brief  Draw a horizontal line
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  length: length of the line
  * @retval None.
  */
void sd1306_draw_fast_Hline(uint8_t x, uint8_t y, uint8_t length, uint8_t color)
{
	while (length > 0)
	{
		sd1306_draw_pixel(x, y, color);
		x++;
		length--;
	}
}

/**
  * @brief  Draw a rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @retval None.
  */
void sd1306_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
	for (uint32_t i = x; i < x + width; i++)
	{
		// Draw the top line of rectangle
		sd1306_draw_pixel(i, y, color);
		// Draw the inferior line of rectangle
		sd1306_draw_pixel(i, y + height, color);
	}
	for (uint32_t i = y; i < y + height; i++)
	{
		// Draw the right line of rectangle
		sd1306_draw_pixel(x, i, color);
		// Draw lthe ledf line of rectangle
		sd1306_draw_pixel(x + width, i, color);
	}
}

/**
  * @brief  Draw a fill rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @retval None.
  */
void sd1306_draw_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
	for (uint32_t i = x; i < x + width; i++)
	{
		for (uint32_t j = y; j < y + height; j++)
		{
			sd1306_draw_pixel(i, j, color);
		}
	}
}

/**
  * @brief  Draw a rounded rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @param  radius: radius of rounded corner
  * @retval None.
  */
void sd1306_draw_round_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color)
{
	// Top
	sd1306_draw_line(x + radius, y, x + width - radius, y, color);
	// Bottom
	sd1306_draw_line(x + radius, y + height - 1, x + width - radius, y + height - 1, color);
	// Left
	sd1306_draw_line(x, y + radius, x, y + height - radius, color);
	// Right
	sd1306_draw_line(x + width - 1, y + radius, x + width - 1, y + height - radius, color);

	// draw four corners

	sd1306_draw_circle_helper(x + radius, y + radius, radius, 1, color);

	sd1306_draw_circle_helper(x + width - radius - 1, y + radius, radius, 2, color);
	sd1306_draw_circle_helper(x + width - radius - 1, y + height - radius - 1, radius, 4, color);
	sd1306_draw_circle_helper(x + radius, y + height - radius - 1, radius, 8, color);

}

/**
  * @brief  Draw a fill rounded rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @param  radius: radius of rounded corner
  * @retval None.
  */
void sd1306_draw_fillround_Rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color)
{
	sd1306_draw_fill_rect(x + radius, y, width - 2 * radius, height, color);

	sd1306_draw_fillcircle_helper(x + width - radius - 1, y + radius, radius, 1, height - 2 * radius - 2, color);
	sd1306_draw_fillcircle_helper(x + radius, y + radius, radius, 2, height - 2 * radius - 2, color);
}

/**
  * @brief  Draw a circle
  * @param  x: x center coordinate
  * @param  y: y center coordinate
  * @param  radius: radius of circle
  * @retval None.
  */
void sd1306_draw_circle(int32_t x, int32_t y, int32_t radius, uint8_t color)
{
	int32_t a;
	int32_t b;
	int32_t P;
	a = 0x00;
	b = radius;
	P = 0x01 - radius;

	do
	{
		{
			sd1306_draw_pixel(a + x, b + y, color);
			sd1306_draw_pixel(b + x, a + y, color);
			sd1306_draw_pixel(x - a, b + y, color);
			sd1306_draw_pixel(x - b, a + y, color);
			sd1306_draw_pixel(b + x, y - a, color);
			sd1306_draw_pixel(a + x, y - b, color);
			sd1306_draw_pixel(x - a, y - b, color);
			sd1306_draw_pixel(x - b, y - a, color);
		}

		if (P < 0)
		{
			P += 3 + 2 * a++;
		}
		else
		{
			P += 5 + 2 * (a++ - b--);
		}

	} while (a <= b);
}

/**
  * @brief  Draw a fill circle
  * @param  x: x center coordinate
  * @param  y: y center coordinate
  * @param  radius: radius of circle
  * @retval None.
  */
void sd1306_draw_fill_circle(int32_t x, int32_t y, int32_t radius, uint8_t color)
{
	int32_t a;
	int32_t b;
	int32_t P;
	a = 0x00;
	b = radius;
	P = 0x01 - radius;

	do
	{
		sd1306_draw_line(x - a, y + b, x + a, y + b, color);
		sd1306_draw_line(x - a, y - b, x + a, y - b, color);
		sd1306_draw_line(x - b, y + a, x + b, y + a, color);
		sd1306_draw_line(x - b, y - a, x + b, y - a, color);

		if (P < 0)
		{
			P += 3 + 2 * a++;
		}
		else
		{
			P += 5 + 2 * (a++ - b--);
		}

	} while (a <= b);
}

/**
  * @brief  Draw a triangle
  * @param  x0: first point's x coordinate
  * @param  y0: first point's y coordinate
  * @param  x1: second point's x coordinate
  * @param  y1: second point's y coordinate
  * @param  x2: third point's x coordinate
  * @param  y2: third point's y coordinate
  * @retval None.
  */
void sd1306_draw_triangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
	sd1306_draw_line(x0, y0, x1, y1, color);
	sd1306_draw_line(x1, y1, x2, y2, color);
	sd1306_draw_line(x2, y2, x0, y0, color);
}

/**
  * @brief  Draw a fill triangle
  * @param  x0: first point's x coordinate
  * @param  y0: first point's y coordinate
  * @param  x1: second point's x coordinate
  * @param  y1: second point's y coordinate
  * @param  x2: third point's x coordinate
  * @param  y2: third point's y coordinate
  * @retval None.
  */
void sd1306_draw_fill_triangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
	int16_t a;
	int16_t b;
	int16_t y;
	int16_t last; 
	int16_t dx01;
	int16_t dy01; 
	int16_t dx02;
	int16_t dy02;
	int16_t dx12;
	int16_t dy12;
	int32_t sa, sb;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1)
	{
		_swap(y0, y1);
		_swap(x0, x1);
	}
	if (y1 > y2)
	{
		_swap(y2, y1);
		_swap(x2, x1);
	}
	if (y0 > y1)
	{
		_swap(y0, y1);
		_swap(x0, x1);
	}

	if (y0 == y2)
	{ // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)
		{
			a = x1;
		}
		else if (x1 > b)
		{
			b = x1;
		}
		if (x2 < a)
		{
			a = x2;
		}
		else if (x2 > b)
		{
			b = x2;
		}
		sd1306_draw_fast_Hline(a, y0, b - a + 1, color);
		return;
	}

	dx01 = x1 - x0;
	dy01 = y1 - y0;
	dx02 = x2 - x0;
	dy02 = y2 - y0;
	dx12 = x2 - x1;
	dy12 = y2 - y1;
	sa = 0;
	sb = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2)
	{
		last = y1;   // Include y1 scanline
	}
	else
	{
		last = y1 - 1; // Skip it
	}

	for (y = y0; y <= last; y++)
	{
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;

		if (a > b)
		{
			_swap(a, b);
		}
		sd1306_draw_fast_Hline(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++)
	{
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;

		if (a > b)
		{
			_swap(a, b);
		}
		sd1306_draw_fast_Hline(a, y, b - a + 1, color);
	}
}

/**
  * @brief  Used to do round rectangles
  * @param  x0: x center coordinate
  * @param  y0: y center coordinate
  * @param  radius: radius
  * @param  cornername: corner to draw the semicircle
  * @retval None.
  */
void sd1306_draw_circle_helper(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t cornername, uint8_t color)
{
	int16_t f = 1 - radius;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * radius;
	int16_t x = 0;
	int16_t y = radius;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4)
		{
			sd1306_draw_pixel(x0 + x, y0 + y, color);
			sd1306_draw_pixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2)
		{
			sd1306_draw_pixel(x0 + x, y0 - y, color);
			sd1306_draw_pixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8)
		{
			sd1306_draw_pixel(x0 - y, y0 + x, color);
			sd1306_draw_pixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1)
		{
			sd1306_draw_pixel(x0 - y, y0 - x, color);
			sd1306_draw_pixel(x0 - x, y0 - y, color);
		}
	}
}

/**
  * @brief  Used to do fill round rectangles
  * @param  x0: x center coordinate
  * @param  y0: y center coordinate
  * @param  radius: radius
  * @param  cornername: corner to draw the semicircle
  * @param  delta: 
  * @retval None.
  */
void sd1306_draw_fillcircle_helper(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t cornername, uint8_t delta, uint8_t color)
{
	int16_t f = 1 - radius;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * radius;
	int16_t x = 0;
	int16_t y = radius;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (cornername & 0x1)
		{
			sd1306_draw_line(x0 + x, y0 - y, x0 + x, y0 - y + 2 * y + 1 + delta, color);
			sd1306_draw_line(x0 + y, y0 - x, x0 + y, y0 - x + 2 * x + 1 + delta, color);
		}
		if (cornername & 0x2)
		{
			sd1306_draw_line(x0 - x, y0 - y, x0 - x, y0 - y + 2 * y + 1 + delta, color);
			sd1306_draw_line(x0 - y, y0 - x, x0 - y, y0 - x + 2 * x + 1 + delta, color);
		}
	}
}


/**
  * @brief  Check if the n bit of a byte is set.
  * @param  data: byte to test
  * @param  n: bit to test
  * @retval 0 if is clear or 1 if is set.
  */
static int32_t  bit_test(uint8_t  data, uint8_t  n)
{
	if (n < 0 || n > 7)
	{
		return  0;
	}
		
	if ((data >> (7 - n)) & 0x1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
  * @brief  Draw a string
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  textptr: pointer 
  * @param  size: scale
  * @retval None.
  */
void sd1306_draw_string(int32_t x, int32_t y, uint8_t* textptr, int32_t size, uint8_t color)
{
	// Loop counters
	uint8_t i;
	uint8_t j;
	uint8_t k;
	uint8_t l; 
	uint8_t m;
	// Stores character data
	uint8_t pixelData[5];

	// Loop through the passed string
	for (i = 0; textptr[i] != 0x00; ++i, ++x)		
	{
		// Get data font
		memcpy(pixelData, font_data[textptr[i] - ' '], 5);

		// Performs character wrapping
		if (x + 5 * size >= 128)				
		{
			// Set x at far left position
			x = 0;		
			// Set y at next position down
			y += 7 * size + 1;							
		}

		// Loop through character byte data
		for (j = 0; j < 5; ++j, x += size)			
		{
			// Loop through the vertical pixels
			for (k = 0; k < 7 * size; ++k)				
			{
				// Check if the pixel should be set
				if (bit_test(BitReverseTable256[pixelData[j]], k))	
				{
					// The next two loops change the character's size
					for (l = 0; l < size; ++l)		
					{									
						for (m = 0; m < size; ++m)
						{
							// Draws the pixel
							sd1306_draw_pixel(x + m, y + k * size + l, color);
						}
					}
				}
			}
		}
	}
}

/**
  * @brief  Set the display upside down.
  * @retval None.
  */
void upside_down(void)
{
	// Set OLED upside down
	sd1306_send_command(sd1306_ADDR, 0xc0);
	// Set segment from right to left
	sd1306_send_command(sd1306_ADDR, 0xa0);
}

/**
  * @brief  Set the display upside up.
  * @retval None.
  */
void upside_up(void)
{
	
	// Set OLED upside up
	sd1306_send_command(sd1306_ADDR, 0xc8);
	// Set segment from left to right
	sd1306_send_command(sd1306_ADDR, 0xa1);
}

/**
  * @brief  Send OLED buffer to OLED RAM 
  * @retval None.
  */
void sd1306_refresh(void)
{
	// Set the lower comulmn address to zero
	sd1306_send_command(sd1306_ADDR, 0x00);
	// Set the higher comulmn address to zero
	sd1306_send_command(sd1306_ADDR, 0x10);
	// Set page address to zero
	sd1306_send_command(sd1306_ADDR, 0xb0);
	// Send OLED buffer to sd1306 RAM
	sd1306_write_data(sd1306_ADDR, oled_buffer);
}

/**
  * @brief  Draw a image in OLED buffer
  * @retval None.
  */
void sd1306_draw_img(const uint8_t * ptr_img)
{
	uint16_t i;
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		oled_buffer[i] = ptr_img[i];
	}
}

/**
  * @brief  Set all buffer's bytes to zero
  * @retval None.
  */
void clear_oled_buffer()
{
	uint16_t i;

	for (i = 0; i < BUFFER_SIZE; i++)
	{
		oled_buffer[i] = 0;
	}
}


/**
  * @brief  Set all buffer's bytes to 0xff
  * @retval None.
  */

void fill_oled_buffer()
{
	uint16_t i;

	for (i = 0; i < BUFFER_SIZE; i++)
	{
		oled_buffer[i] = 0xff;
	}
}

/**
  * @brief  Draw an arc given angles (This is jus a test function, not optimized)
  * @param x: x coordinate of the center
  * @param y: y coordinate of the center
  * @param radius: radius of arc
  * @param a0: start angle
  * @param a1: end angle
  * @retval None.
  */
void sd1306_draw_arc(int32_t x, int32_t y, int32_t radius, int32_t a0, int32_t a1, uint8_t color)
{
	int32_t a, b, P;
	a = 0x00;
	b = radius;
	P = 0x01 - radius;

	int32_t angle;

	do
	{


		angle = atan2f(b, a)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(a + x, y - b, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(a + x, y - b, color);
			}
		}



		angle = atan2f(a, b)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(b + x, y - a, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(b + x, y - a, color);
			}
		}



		angle = atan2f(b, -a)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(x - a, y - b, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(x - a, y - b, color);
			}
		}


		angle = atan2f(a, -b)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(x - b, y - a, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(x - b, y - a, color);
			}
		}


		angle = atan2f(-a, b)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(b + x, y + a, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(b + x, y + a, color);
			}
		}



		angle = atan2f(-b, a)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(a + x, y + b, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(a + x, y + b, color);
			}
		}



		angle = atan2f(-b, -a)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(x - a, y + b, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(x - a, y + b, color);
			}
		}



		angle = atan2f(-a, -b)*180.0 / 3.14;

		angle < 0 ? angle += 360 : angle;

		if (a1 > a0)
		{
			if (angle >= a0 && angle <= a1)
			{
				sd1306_draw_pixel(x - b, y + a, color);
			}
		}
		else
		{
			if ((angle >= a0 && angle < 360) || (angle <= a1 && angle >= 0))
			{
				sd1306_draw_pixel(x - b, y + a, color);
			}
		}

		if (P < 0)
		{
			P += 3 + 2 * a++;
		}
		else
		{
			P += 5 + 2 * (a++ - b--);
		}

	} while (a <= b);
}