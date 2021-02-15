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

#pragma once
#ifndef OLED_H
#define OLED_H



#include <stdio.h>
#include <stdint.h>
#include "sd1306.h"
#include "applibs_versions.h"
#include <applibs/wificonfig.h>
#include "deviceTwin.h"


#define OLED_NUM_SCREEN 7

#define OLED_TITLE_X      0
#define OLED_TITLE_Y      0 
#define OLED_RECT_TITLE_X 0
#define OLED_RECT_TITLE_Y 0
#define OLED_RECT_TITLE_W 127
#define OLED_RECT_TITLE_H 18

#define OLED_LINE_1_X     0
#define OLED_LINE_1_Y     16

#define OLED_LINE_2_X     0
#define OLED_LINE_2_Y     26

#define OLED_LINE_3_X     0
#define OLED_LINE_3_Y     36


#define OLED_LINE_4_X     0
#define OLED_LINE_4_Y     46

#define FONT_SIZE_TITLE   2
#define FONT_SIZE_LINE    1

#define SSID_MAX_LEGTH    15


const unsigned char Image_avnet_bmp[1024];

extern int8_t oled_state;

enum oledStateEnum {
	BUS_STATUS = 0,
	NETWORK_STATUS,
	CLOUD_MESSAGE,
	ACCEL_DATA,
	ANGULAR_RATE_DATA,
	ENVIRONMENT,
	OTHER,
	LOGO
};

enum oledBusStatusEnum {
	CLEAR_BUFFER = 0,
	LSM6DSO_STATUS_DISPLAY,
	LPS22HH_STATUS_DISPLAY,
	I2C_INIT
};

typedef struct
{
	uint8_t SSID[WIFICONFIG_SSID_MAX_LENGTH];
	uint32_t frequency_MHz;
	int8_t rssi;
} network_var;

extern network_var network_data;

extern float light_sensor;

extern uint8_t oled_init(void);

extern void oled_i2c_bus_status(uint8_t lsmod_status);

extern void update_oled(void);

extern void oled_draw_logo(void);



void update_network(void);
void update_accel(float x, float y, float z);
void update_angular_rate(float x, float y, float z);
void update_environ(float temp1, float temp2, float atm);
void update_other(float x, float y, float z);

/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  n: float number to convert
  * @param  res:
  * @param  afterpoint:
  * @retval None.
  */
extern void ftoa(float n, uint8_t *res, int32_t afterpoint);

/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  x: x integer input
  * @param  str: uint8_t array output
  * @param  d: Number of zeros added
  * @retval i: number of digits
  */
extern int32_t intToStr(int32_t x, uint8_t str[], int32_t d);

uint8_t get_str_size(uint8_t * str);

#endif
