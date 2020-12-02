#ifndef __ARDUCAM_H
#define __ARDUCAM_H

#include "exit_codes.h"
#include <applibs/gpio.h>
#include <stdlib.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/networking.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <applibs/storage.h>
#include "arducam_driver/ArduCAM.h"
#include "delay.h"

extern int senbLedGpioFd;

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
//typedef enum {
//    ExitCode_Arducam_Not_Found = 13

    // Reserve ExitCodes 13 - 20 for ArduCam specific codes defined in arducam.h
//} ExitCode2;

// Verify we're building a valid configuration
#if (defined(CFG_MODE_JPEG) && defined(CFG_MODE_BITMAP)) || \
    (!defined(CFG_MODE_JPEG) && !defined(CFG_MODE_BITMAP))
#error "define CFG_MODE_JPEG or CFG_MODE_BITMAP"
#endif
#if defined(CFG_MODE_JPEG)
#define FILE_EXTENSION ".jpg"
#elif defined(CFG_MODE_BITMAP)
#define BMPIMAGEOFFSET 66
const uint8_t bmp_header[BMPIMAGEOFFSET] = {
    0x42, 0x4D,                 // MagicNumber = 'B', 'M'
    0x42, 0x58, 0x02, 0x00,     // FileSize = 320x240x2 + 66
    // 0x42, 0x96, 0x00, 0x00,  // FileSize = 160x120x2 + 66
    0x00, 0x00, 0x00, 0x00,     // Reserved
    0x42, 0x00, 0x00, 0x00,     // Pixel Offset in memory = 66
    0x28, 0x00, 0x00, 0x00,     // BitmapInfoHeaderSize = 40
    0x40, 0x01, 0x00, 0x00,     // W = 320
    // 0xA0, 0x00, 0x00, 0x00,  // W = 320
    0xF0, 0x00, 0x00, 0x00,     // H = 240
    // 0x78, 0x00, 0x00, 0x00,  // H = 240
    0x01, 0x00,                 // Plane
    0x10, 0x00,                 // 16bit RG
    0x03, 0x00, 0x00, 0x00,     // Compression = BI_BITFIELDS(3)
    0x00, 0x58, 0x02, 0x00,     // ImageSize = 320x240x2
    // 0x00, 0x96, 0x00, 0x00,  // ImageSize = 160x120x2
    0x00, 0x00, 0x00, 0x00,     // XPelsPerMeter
    0x00, 0x00, 0x00, 0x00,     // YPelsPerMeter
    0x00, 0x00, 0x00, 0x00,     // biClrUsed
    0x00, 0x00, 0x00, 0x00,     // biClrImportant
    0x00, 0xF8, 0x00, 0x00,     // Red mask
    0xE0, 0x07, 0x00, 0x00,     // Green mask
    0x1F, 0x00, 0x00, 0x00      // Blue mask
};
#endif

ExitCode arduCamInit(int csGPIO, int spiISU, int i2cISU);
void UploadFileToAzureBlob(uint32_t img_len);
uint32_t CaptureImage(void);

#endif // __ARDUCAM_H