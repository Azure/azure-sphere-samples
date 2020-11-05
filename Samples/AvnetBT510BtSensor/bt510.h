/* Copyright (c) Avnet Corporation. All rights reserved.
   Licensed under the MIT License. */

/*
This file implements routines requied to parse BT510 advertisement messages received over a uart interface.
*/

// BW To Do List
// Architect and document IoTConnect implementation
// Add telemetry for all alarms
// Add Send device twin for new BT510s
//      Firmware version
//      Boot Loader version
//      Address
//      Name
//      Other stuff?
// Only send device twins stuff once per boot
// Add telemetry for magnet events
// Add telemetry for battery events

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

// Define the Json string for reporting BT510 telemetry data
static const char bt510TelemetryJsonObject[] =
    "{\"%s\":{\"BT510Address\":\"%s\",\"rssi\":\"%s\",\"temp\":%2.2f}}";

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

// BT510 Global variables

// Array to hold specific data for each BT510 detected by the system
typedef struct BT510Device {
    char bdAddress[18];
    uint16_t recordNumber;
    bool lastContactIsOpen;
} BT510Device_t;

extern void SendTelemetry(const char *jsonMessage, const char *propertyName,
                          const char *propertyValue);

// BT510 Specific routines
int stringToInt(char *, size_t);
void textFromHexString(char *, char *, int);
void getDeviceName(char *, BT510Message_t *);
void getBdAddress(char *, BT510Message_t *);
void getFirmwareVersion(char *, BT510Message_t *);
void getBootloaderVersion(char *, BT510Message_t *);
void getRxRssi(char *rxRssi, BT510Message_t *);
void parseFlags(uint16_t);
int getBT510DeviceIndex(char *);
int addBT510DeviceToList(char *, BT510Message_t *);
