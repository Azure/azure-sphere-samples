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
#ifndef HEADER_sd1306_H
#define HEADER_sd1306_H



#include <stdint.h>
#include "i2c.h"
#include <applibs/i2c.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


#define sd1306_ADDR 0x3c


#define OLED_HEIGHT 64
#define OLED_WIDTH  128
#define BUFFER_SIZE OLED_HEIGHT*OLED_WIDTH/8

#define _swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) 

enum pixelcolor
{
	black_pixel,
	white_pixel,
	inverse_pixel,
};

/**
  * @brief  Initialize sd1306.
  * @param  None.
  * @retval None.
  */
extern uint8_t sd1306_init(void);

/**
  * @brief  Draw a pixel at specified coordinates
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @retval None.
  */
void sd1306_draw_pixel(int32_t x, int32_t y, uint8_t color);

/**
  * @brief  Draw a line
  * @param  x1: x coordinate of start point
  * @param  y1: y coordinate of start point
  * @param  x2: x coordinate of end point
  * @param  y2: y coordinate of end point
  * @retval None.
  */
extern void sd1306_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
  * @brief  Draw a vertical line
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  length: length of the line
  * @retval None.
  */
extern void sd1306_draw_fast_Vline(uint8_t x, uint8_t y, uint8_t length, uint8_t color);

/**
  * @brief  Draw a horizontal line
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  length: length of the line
  * @retval None.
  */
extern void sd1306_draw_fast_Hline(uint8_t x, uint8_t y, uint8_t length, uint8_t color);

/**
  * @brief  Draw a rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @retval None.
  */
extern void sd1306_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

/**
  * @brief  Draw a fill rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @retval None.
  */
extern void sd1306_draw_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

/**
  * @brief  Draw a rounded rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @param  radius: radius of rounded corner
  * @retval None.
  */
extern void sd1306_draw_round_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);

/**
  * @brief  Draw a fill rounded rectangle given start point, width and height
  * @param  x: x coordinate
  * @param  y: y coordinate
  * @param  width: rectangle width
  * @param  height: rectangle height
  * @param  radius: radius of rounded corner
  * @retval None.
  */
extern void sd1306_draw_fillround_Rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);

/**
  * @brief  Draw a circle
  * @param  x: x center coordinate
  * @param  y: y center coordinate
  * @param  radius: radius of circle
  * @retval None.
  */
extern void sd1306_draw_circle(int32_t x, int32_t y, int32_t radius, uint8_t color);

/**
  * @brief  Draw a fill circle
  * @param  x: x center coordinate
  * @param  y: y center coordinate
  * @param  radius: radius of circle
  * @retval None.
  */
extern void sd1306_draw_fill_circle(int32_t x, int32_t y, int32_t radius, uint8_t color);

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
extern void sd1306_draw_triangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

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
extern void sd1306_draw_fill_triangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
  * @brief  Draw a string
  * @param  x: x coordinate of start point
  * @param  y: y coordinate of start point
  * @param  textptr: pointer
  * @param  size: scale
  * @retval None.
  */
extern void sd1306_draw_string(int32_t x, int32_t y, uint8_t* textptr, int32_t size, uint8_t color);

/**
  * @brief  Used to do round rectangles
  * @param  x0: x center coordinate
  * @param  y0: y center coordinate
  * @param  radius: radius
  * @param  cornername: corner to draw the semicircle
  * @retval None.
  */
void sd1306_draw_circle_helper(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t cornername, uint8_t color);

/**
  * @brief  Used to do fill round rectangles
  * @param  x0: x center coordinate
  * @param  y0: y center coordinate
  * @param  radius: radius
  * @param  cornername: corner to draw the semicircle
  * @param  delta:
  * @retval None.
  */
void sd1306_draw_fillcircle_helper(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t cornername, uint8_t delta, uint8_t color);

/**
  * @brief  Set the display upside up.
  * @retval None.
  */
extern void upside_up(void);

/**
  * @brief  Set the display upside down.
  * @retval None.
  */
extern void upside_down(void);

/**
  * @brief  Send OLED buffer to OLED RAM
  * @retval None.
  */
extern void sd1306_refresh(void);

/**
  * @brief  Draw a image in OLED buffer
  * @retval None.
  */
extern void sd1306_draw_img(const uint8_t * ptr_img);

/**
  * @brief  Set all buffer's bytes to zero
  * @retval None.
  */
extern void clear_oled_buffer(void);

/**
  * @brief  Set all buffer's bytes to 0xff
  * @retval None.
  */
extern void fill_oled_buffer(void);

/**
  * @brief  Draw an arc given angles (This is jus a test function, not optimized)
  * @param x: x coordinate of the center
  * @param y: y coordinate of the center
  * @param radius: radius of arc
  * @param a0: start angle
  * @param a1: end angle
  * @retval None.
  */
extern void sd1306_draw_arc(int32_t x, int32_t y, int32_t radius, int32_t a0, int32_t a1, uint8_t color);



#endif