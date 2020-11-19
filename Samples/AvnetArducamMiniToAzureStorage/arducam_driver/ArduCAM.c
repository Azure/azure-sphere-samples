/*
  ArduCAM.cpp - Arduino library support for CMOS Image Sensor
  Copyright (C)2011-2015 ArduCAM.com. All right reserved

  Basic functionality of this library are based on the demo-code provided by
  ArduCAM.com. You can find the latest version of the library at
  http://www.ArduCAM.com
*/
#include <stdint.h>
#include <stdbool.h>

#include "../delay.h"
#include "ll.h"
#include "ArduCAM.h"

#ifdef USE_OV2640
#include "ov2640_regs.h"
#endif 
#ifdef USE_OV5642
#include "ov5642_regs.h"
#endif 

static uint8_t m_fmt;

static uint8_t get_bit(uint8_t addr, uint8_t bit);
static void write_reg(uint8_t addr, uint8_t data);
static uint8_t read_reg(uint8_t addr);

static void wrSensorRegs8_8(const struct sensor_reg[]);
static void wrSensorRegs8_16(const struct sensor_reg[]);
static void wrSensorRegs16_8(const struct sensor_reg[]);
static void wrSensorRegs16_16(const struct sensor_reg[]);
static void wrSensorReg8_8(uint8_t regID, uint8_t regDat);
static void wrSensorReg8_16(uint8_t regID, uint16_t regDat);
static void wrSensorReg16_8(uint16_t regID, uint8_t regDat);
static void wrSensorReg16_16(uint16_t regID, uint16_t regDat);
static void rdSensorReg8_8(uint8_t regID, uint8_t* regDat);
static void rdSensorReg16_8(uint16_t regID, uint8_t* regDat);
static void rdSensorReg8_16(uint8_t regID, uint16_t* regDat);
static void rdSensorReg16_16(uint16_t regID, uint16_t* regDat);

void arducam_CS_LOW(void)
{
	ll_gpio_cs_go_low();
}

void arducam_CS_HIGH(void)
{
	ll_gpio_cs_go_high();
}

static uint8_t get_bit(uint8_t addr, uint8_t bit)
{
    uint8_t temp;
    temp = read_reg(addr);
    temp = temp & bit;
    return temp;
}

static void write_reg(uint8_t addr, uint8_t data)
{
    uint8_t txBuf[2];

    txBuf[0] = addr | 0x80;
    txBuf[1] = data;

	ll_gpio_cs_go_low();
    ll_spi_tx(&txBuf[0], 2);
	ll_gpio_cs_go_high();
}

static uint8_t read_reg(uint8_t addr)
{
    uint8_t txBuf[2], rxBuf[2];

    txBuf[0] = addr & 0x7F;
    txBuf[1] = 0x00;

	ll_gpio_cs_go_low();
    ll_spi_tx_then_rx(&txBuf[0], 1, &rxBuf[0], 1);
	ll_gpio_cs_go_high();

    return rxBuf[0];
}

void arducam_ll_init(void)
{
	ll_gpio_init();
    ll_i2c_init();
    ll_spi_init();
}

void arducam_flush_fifo(void)
{
    write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

void arducam_start_capture(void)
{
    write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

void arducam_clear_fifo_flag(void)
{
    write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

uint32_t arducam_read_fifo_length(void)
{
    uint32_t len1, len2, len3, length;
    len1 = read_reg(FIFO_SIZE1);
    len2 = read_reg(FIFO_SIZE2);
    len3 = read_reg(FIFO_SIZE3) & 0x7f;
    length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
    return length;
}

void arducam_set_fifo_burst(void)
{
	// call arducam_CS_LOW() first
	uint8_t txBuf = BURST_FIFO_READ;	
    ll_spi_tx(&txBuf, 1);
}

void arducam_read_fifo_burst(uint8_t* rxBuf, uint32_t length)
{
	ll_spi_rx(rxBuf, length);
}

int arducam_test(void)
{
    uint8_t reg, vid, pid;

    // Check SPI and communication to CPLD
    write_reg(ARDUCHIP_TEST1, 0x55);
    reg = read_reg(ARDUCHIP_TEST1);

    if (reg != 0x55) {
        return -1;
    }

#ifdef USE_OV2640
    // Check I2C and communication to OV2640
    wrSensorReg8_8(0xff, 0x01);
    rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42))) {
        return -1;
    }

#endif 

#ifdef USE_OV5642
    #define OV5642_CHIPID_HIGH 0x300a
    #define OV5642_CHIPID_LOW 0x300b

    // Check I2C and communication to OV5642
//    wrSensorReg8_8(0xff, 0x01);  
    rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
//	if ((vid != 0x56) && ((pid != 0x41) || (pid != 0x42))) {
    if ((vid != 0x42) && ((pid != 0x41) || (pid != 0x42))) {

        return -1;
    }

#endif 

    return 0;
}

void arducam_reset(void)
{
	write_reg(0x07, 0x80);
	delay_ms(100);
	write_reg(0x07, 0x00);
	delay_ms(100);
}

bool arducam_check_fifo_done(void)
{
	if (get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) == CAP_DONE_MASK) {
		return true;
	} else {
		return false;
	}
}

void wrSensorReg8_8(uint8_t regID, uint8_t regDat)
{
    uint8_t buf[2];
	buf[0] = regID;
	buf[1] = regDat;

    ll_i2c_tx(&buf[0], 2);

    delay_ms(1);
}

void wrSensorReg8_16(uint8_t regID, uint16_t regDat)
{
	uint8_t txBuf[3];
	txBuf[0] = regID;
	txBuf[1] = (uint8_t)((regDat >> 8) & 0x0FF);
	txBuf[2] = (uint8_t)(regDat & 0x00FF);

    ll_i2c_tx(&txBuf[0], 3);

    delay_ms(1);
}

void wrSensorReg16_8(uint16_t regID, uint8_t regDat)
{
	uint8_t txBuf[3];
	txBuf[0] = (uint8_t)((regID >> 8) & 0x00FF);
	txBuf[1] = (uint8_t)(regID & 0x00FF);
	txBuf[2] = regDat;

    ll_i2c_tx(&txBuf[0], 3);
   
    delay_ms(1);
}

void wrSensorReg16_16(uint16_t regID, uint16_t regDat)
{
	uint8_t txBuf[4];
	txBuf[0] = (uint8_t)((regID >> 8) & 0x00FF);
	txBuf[1] = (uint8_t)(regID & 0x00FF);
	txBuf[2] = (uint8_t)((regDat >> 8) & 0x0FF);
	txBuf[3] = (uint8_t)(regDat & 0x00FF);

    ll_i2c_tx(&txBuf[0], 4);
   
    delay_ms(1);
}

void wrSensorRegs8_8(const struct sensor_reg reglist[])
{
    uint8_t reg_addr = 0;
    uint8_t reg_val = 0;
    const struct sensor_reg *next = reglist;
    while ((reg_addr != 0xff) | (reg_val != 0xff))
    {
        reg_addr = (uint8_t)next->reg;
        reg_val = (uint8_t)next->val;
        wrSensorReg8_8(reg_addr, reg_val);
        next++;
    }
}

void wrSensorRegs8_16(const struct sensor_reg reglist[])
{
    uint8_t  reg_addr = 0;
    uint16_t reg_val  = 0;
    const struct sensor_reg *next = reglist;

    while ((reg_addr != 0xff) | (reg_val != 0xffff))
    {
        reg_addr = (uint8_t)next->reg;
        reg_val = next->val;
        wrSensorReg8_16(reg_addr, reg_val);
        next++;
    }
}

void wrSensorRegs16_8(const struct sensor_reg reglist[])
{
    uint16_t reg_addr = 0;
    uint8_t  reg_val  = 0;
    const struct sensor_reg *next = reglist;

    while ((reg_addr != 0xffff) | (reg_val != 0xff))
    {
        reg_addr = next->reg;
        reg_val = (uint8_t)next->val;
        wrSensorReg16_8(reg_addr, reg_val);
        next++;
    }
}

void wrSensorRegs16_16(const struct sensor_reg reglist[])
{
    uint16_t reg_addr, reg_val = 0;
    const struct sensor_reg *next = reglist;

    while ((reg_addr != 0xffff) | (reg_val != 0xffff))
    {
        reg_addr = next->reg;
        reg_val = next->val;
        wrSensorReg16_16(reg_addr, reg_val);
        next++;
    }
}

void rdSensorReg8_8(uint8_t regID, uint8_t* regDat)
{
    ll_i2c_tx_then_rx(&regID, 1, regDat, 1);

    delay_ms(1);
}

void rdSensorReg16_8(uint16_t regID, uint8_t* regDat)
{
    uint8_t txBuf[2];
	txBuf[0] = (uint8_t)((regID >> 8) & 0x00FF);
	txBuf[1] = (uint8_t)(regID & 0x00FF);

    ll_i2c_tx_then_rx(txBuf, 2, regDat, 1);

    delay_ms(1);
}

void rdSensorReg8_16(uint8_t regID, uint16_t* regDat)
{
    uint8_t rxBuf[2];

    ll_i2c_tx_then_rx(&regID, 1, &rxBuf[0], 2);
    *regDat = (uint16_t)((rxBuf[0] << 8) | rxBuf[1]);
    
    delay_ms(1);
}

void rdSensorReg16_16(uint16_t regID, uint16_t* regDat)
{
    uint8_t txBuf[2];
    uint8_t rxBuf[2];

	txBuf[0] = (uint8_t)((regID >> 8) & 0x00FF);
	txBuf[1] = (uint8_t)(regID & 0x00FF);

    ll_i2c_tx_then_rx(txBuf, 2, rxBuf, 2);
    *regDat = (uint16_t)(rxBuf[0] << 8 | rxBuf[1]);

    delay_ms(1);
}

#ifdef USE_OV2640
// API: OV2640_set_JPEG_size
void arducam_OV2640_set_JPEG_size(uint8_t size)
{
    switch (size)
    {
    case OV2640_160x120:
        wrSensorRegs8_8(OV2640_160x120_JPEG);
        break;
    case OV2640_176x144:
        wrSensorRegs8_8(OV2640_176x144_JPEG);
        break;
    case OV2640_320x240:
        wrSensorRegs8_8(OV2640_320x240_JPEG);
        break;
    case OV2640_352x288:
        wrSensorRegs8_8(OV2640_352x288_JPEG);
        break;
    case OV2640_640x480:
        wrSensorRegs8_8(OV2640_640x480_JPEG);
        break;
    case OV2640_800x600:
        wrSensorRegs8_8(OV2640_800x600_JPEG);
        break;
    case OV2640_1024x768:
        wrSensorRegs8_8(OV2640_1024x768_JPEG);
        break;
    case OV2640_1280x1024:
        wrSensorRegs8_8(OV2640_1280x1024_JPEG);
        break;
    case OV2640_1600x1200:
        wrSensorRegs8_8(OV2640_1600x1200_JPEG);
        break;
    default:
        wrSensorRegs8_8(OV2640_320x240_JPEG);
        break;
    }
}
#endif 
#ifdef USE_OV5642
void arducam_OV5642_set_JPEG_size(uint8_t size)
{
    uint8_t reg_val;

    switch (size) {
    case OV5642_320x240:
        wrSensorRegs16_8(ov5642_320x240);
        break;
    case OV5642_640x480:
        wrSensorRegs16_8(ov5642_640x480);
        break;
    case OV5642_1024x768:
        wrSensorRegs16_8(ov5642_1024x768);
        break;
    case OV5642_1280x960:
        wrSensorRegs16_8(ov5642_1280x960);
        break;
    case OV5642_1600x1200:
        wrSensorRegs16_8(ov5642_1600x1200);
        break;
    case OV5642_2048x1536:
        wrSensorRegs16_8(ov5642_2048x1536);
        break;
    case OV5642_2592x1944:
        wrSensorRegs16_8(ov5642_2592x1944);
        break;
    default:
        wrSensorRegs16_8(ov5642_320x240);
        break;
    }
}
#endif 
// API: set_format
void arducam_set_format(uint8_t fmt)
{
    if (fmt == BMP) {
        m_fmt = BMP;
    } else {
        m_fmt = JPEG;
    }
}

// API: InitCAM
void arducam_InitCAM()
{

#if defined (USE_OV2640)

    wrSensorReg8_8(0xff, 0x01);
    wrSensorReg8_8(0x12, 0x80);

    delay_ms(100);

    if (m_fmt == JPEG)
    {
        wrSensorRegs8_8(OV2640_JPEG_INIT);
        wrSensorRegs8_8(OV2640_YUV422);
        wrSensorRegs8_8(OV2640_JPEG);
        wrSensorReg8_8(0xff, 0x01);
        wrSensorReg8_8(0x15, 0x00);
        wrSensorRegs8_8(OV2640_320x240_JPEG);
        //wrSensorReg8_8(0xff, 0x00);
        //wrSensorReg8_8(0x44, 0x32);
    }
    else
    {
        wrSensorRegs8_8(OV2640_QVGA);
    }
#endif 
#if defined(USE_OV5642)

    wrSensorReg16_8(0x3008, 0x80);
    wrSensorRegs16_8(OV5642_QVGA_Preview);

    if (m_fmt == JPEG) {
        wrSensorRegs16_8(OV5642_JPEG_Capture_QSXGA);
        wrSensorRegs16_8(ov5642_320x240);
        wrSensorReg16_8(0x3818, 0xa8);
        wrSensorReg16_8(0x3621, 0x10);
        wrSensorReg16_8(0x3801, 0xb0);
        wrSensorReg16_8(0x4407, 0x04);
    } else {
        uint8_t reg_val;
        wrSensorReg16_8(0x4740, 0x21);
        wrSensorReg16_8(0x501e, 0x2a);
        wrSensorReg16_8(0x5002, 0xf8);
        wrSensorReg16_8(0x501f, 0x01);
        wrSensorReg16_8(0x4300, 0x61);
        rdSensorReg16_8(0x3818, &reg_val);
        wrSensorReg16_8(0x3818, (reg_val | 0x60) & 0xff);
        rdSensorReg16_8(0x3621, &reg_val);
        wrSensorReg16_8(0x3621, reg_val & 0xdf);
    }
    write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK); // VSYNC is active HIGH

    uint8_t _x3503;
    wrSensorReg16_8(0x5001, _x3503 | 0x01); // Close auto exposure mode
                                            // Manually set the exposure value
    wrSensorReg16_8(0x3500, 0x00);
    wrSensorReg16_8(0x3501, 0x79);
    wrSensorReg16_8(0x3502, 0xe0);		
#endif 
}
