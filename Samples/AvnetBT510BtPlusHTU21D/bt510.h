/* 
MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

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


/*
This file implements routines requied to parse BT510 advertisement messages received over a uart interface.
*/

// BW To Do List
// Architect and document IoTConnect implementation
// OTA updates for BLE PMOD
//      Other stuff?

// Document required production features
// 1. Configure devices
// 2. Configure IoTConnect to know about devices
// 3. Way to black/white list BT510 devices in case there are multiple Azure Sphere devices that can see the BT510 messages

#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include "build_options.h"

// Enable this define to send test messages to the parser from main.c line ~1190
//#define ENABLE_MESSAGE_TESTING

// Enable this define to see more debug around the message parsing
//#define ENABLE_MSG_DEBUG

// Define the Json string for reporting BT510 telemetry data
static const char bt510MagnetTelemetryJsonObject[] = "\"magnet%s\":%d,";
static const char bt510TemperatureJsonObject[] = "\"temp%s\":%2.2f,";
static const char bt510BatteryJsonObject[] = "\"bat%s\":%2.3f,";
static const char bt510RssiJsonObject[] = "\"rssi%s\":%d,";
static const char htu21dTempHumidityJsonObject[] = "{\"tempHTU21D\":%2.2f,\"humidityHTU21D\":%2.2f,";
//"{\"temp\":%2.2f, \"humidity\":%2.2f,";

// Magnet related message
static const char bt510MagnetEventTelemetryJsonObject[] = "{\"magnet%s\":%d}";

// Movement related message
static const char bt510MovementTelemetryJsonObject[] = "{\"movement%s\":1}";

// Reset message
static const char bt510ResetAlarmTelemetryJsonObject[] = "{\"reset%s\":\"true\",\"resetEnum%s\":%d}";

// Initial device twin message with device details captured
#ifdef TARGET_QIIO_200
static const char bt510DeviceTwinsonObject[] = "{\"BT510deviceName\":\"%s\",\"BT510mac\":\"%s\",\"BT510fwVersion\":\"%s\",\"BT510bootloaderVersion\":\"%s\"}";
#else
static const char bt510DeviceTwinsonObject[] = "{\"deviceName%s\":\"%s\",\"mac%s\":\"%s\",\"fwVersion%s\":\"%s\",\"bootloaderVersion%s\":\"%s\"}";
#endif

/*
Note to work with IoTConnect as a gateway, we need to implement/send this JSON

[{
                "id": "gateway1",
                "d": []
        },
        {
                "id": "BT510-1",
                "d": [{
                        "mac": "E7-E0-E9-02-95-A5",
                        "rssi": -74,
                        "temperature": 22.4,
                        "contact": "closed"
                }]
        },
        {
                "id": "BT510-2",
                "d": [{
                        "mac": "E7-E0-E9-02-95-A5",
                        "rssi": -74,
                        "temperature": 22.4,
                        "contact": "closed"
                }]
        }
]
*/


// Define the maximum length of the device name as pulled from the message
#define MAX_NAME_LENGTH 24

//    BS1:3129FF7700520003010100000280946E479C72C91107000800000000000000000000030007000001000D000609425435313000 -53

// Define the content of the message from the BT510.
typedef struct BT510Message {
    char msgSendRxId[3]; // BS1 or BR1
    uint8_t msgColon[1];
    uint8_t ignore[1 * 2];
    uint8_t msgLength[1 * 2];
    uint8_t mfgType[1 * 2];         // 0xFF
    uint8_t companyId[2 * 2];       // 0x0077
    uint8_t protocolId[2 * 2];      // 0x0052
    uint8_t repeatHeaderLen[1 * 2]; // 0x03
    uint8_t currentTTLCount[1 * 2];
    uint8_t maxTTLCount[1 * 2];
    uint8_t networkId[2 * 2];
    uint8_t flags[2 * 2];
    uint8_t BdAddress[6 * 2];
    uint8_t recordType[1 * 2];
    uint8_t recordNumber[2 * 2];
    uint8_t epoc[2 * 4];
    uint8_t data[2 * 4];
    uint8_t resetCount[1 * 2];
    uint8_t productId[2 * 2];
    uint8_t firmwareVersion[3 * 2];
    uint8_t firmwareType[1 * 2];
    uint8_t configVersion[1 * 2];
    uint8_t bootLoaderVersion[3 * 2];
    uint8_t hardwareVersion[1 * 2];
    uint8_t deviceNameLength[1 * 2];
    uint8_t deviceNameId[1 * 2]; // 0x08 or 0x09
    char deviceNameString[24 * 2];
} BT510Message_t;

// Define the record types, these define what data is included with the message
 enum recordType {
    RT_RESERVED0 = 0,
    RT_TEMPERATURE,
    RT_MAGNET,
    RT_MOVEMENT,
    RT_ALARM_HIGH_TEMP1,
    RT_ALARM_HIGH_TEMP2,
    RT_ALARM_HIGH_TEMP_CLEAR,
    RT_ALARM_LOW_TEMP1,
    RT_ALARM_LOW_TEMP2,
    RT_ALARM_LOW_TEMP_CLEAR,
    RT_ALARM_DELTA_TEMP,
    RT_SKIP_A_ENUM,
    RT_BATTERY_GOOD,
    RT_ADVERTISE_ON_BUTTON,
    RT_RESERVED1,
    RT_RESERVED2,
    RT_BATTERY_BAD,
    RT_RESET
};

// Define each bit in the flags uint16_t
enum flag_enum {
    FLAG_RTC_SET = 0,
    FLAG_ACTIVITY_MODE,
    FLAG_ANY_FLAG_WAS_SET,
    FLAG_RESERVED0,
    FLAG_RESERVED1,
    FLAG_RESERVED2,
    FLAG_RESERVED3,
    FLAG_LOW_BATTERY_ALARM,
    FLAG_HIGH_TEMP_ALARM_BIT0,
    FLAG_HIGH_TEMP_ALARM_BIT1,
    FLAG_LOW_TEMP_ALARM_BIT0,
    FLAG_LOW_TEMP_ALARM_BIT1,
    FLAG_DELTA_TEMP_ALARM,
    FLAG_RESERVED4,
    FLAG_MOVEMENT_ALARM,
    FLAG_MAGNET_STATE
};

#define MAX_BT510_DEVICES 10
#define BT510_ADDRESS_LEN 18

// BT510 Global variables

// Array to hold specific data for each BT510 detected by the system
typedef struct BT510Device {
    char bt510Name[MAX_NAME_LENGTH + 1];
    char bdAddress[BT510_ADDRESS_LEN];
    uint16_t recordNumber;
    bool lastContactIsOpen;
    float lastTemperature;
    float lastBattery;
    int lastRssi;
} BT510Device_t;

extern void SendTelemetry(const char *, bool);
extern void TwinReportState(const char *jsonState);
extern BT510Device_t BT510DeviceList[MAX_BT510_DEVICES];
extern char authorizedDeviceList[MAX_BT510_DEVICES][BT510_ADDRESS_LEN];

// BT510 Specific routines
int stringToInt(char *, size_t);
void textFromHexString(char *, char *, int);
void getDeviceName(char *, BT510Message_t *);
void getBdAddress(char *, BT510Message_t *);
void getFirmwareVersion(char *, BT510Message_t *);
void getBootloaderVersion(char *, BT510Message_t *);
void getRxRssi(char*, char*);
void parseFlags(uint16_t);
int getBT510DeviceIndex(char *);
int addBT510DeviceToList(char *, BT510Message_t *);
void processData(int, int);
bool isDeviceAuthorized(char *);
void bt510SendTelemetry(void);