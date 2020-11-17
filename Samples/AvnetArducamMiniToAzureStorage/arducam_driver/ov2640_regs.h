#ifndef __OV2640_REGS_H
#define __OV2640_REGS_H

#include "ArduCAM.h"

#define OV2640_CHIPID_HIGH 	0x0A
#define OV2640_CHIPID_LOW 	0x0B

extern const struct sensor_reg OV2640_QVGA[];
extern const struct sensor_reg OV2640_QQVGA[];
extern const struct sensor_reg OV2640_JPEG_INIT[];
extern const struct sensor_reg OV2640_YUV422[];
extern const struct sensor_reg OV2640_JPEG[];
extern const struct sensor_reg OV2640_160x120_JPEG[];
extern const struct sensor_reg OV2640_176x144_JPEG[];
extern const struct sensor_reg OV2640_320x240_JPEG[];
extern const struct sensor_reg OV2640_352x288_JPEG[];
extern const struct sensor_reg OV2640_640x480_JPEG[];
extern const struct sensor_reg OV2640_800x600_JPEG[];
extern const struct sensor_reg OV2640_1024x768_JPEG[];
extern const struct sensor_reg OV2640_1280x1024_JPEG[];
extern const struct sensor_reg OV2640_1600x1200_JPEG[];
 
#endif

