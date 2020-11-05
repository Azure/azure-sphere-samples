#include "bt510.h"

// Global variables
BT510Device_t BT510DeviceList[10];
int currentBT510DeviceIndex = -1;
int numBT510DevicesInList = 0;
char deviceName[MAX_NAME_LENGTH + 1];
char bdAddress[] = "  -  -  -  -  -  \0";
char firmwareVersion[] = "  .  .  \0";
char bootloaderVersion[] = "  .  .  \0";
char rxRssi[] = "-xx\0";
uint32_t sensorData;
uint16_t sensorFlags;
// uint16_t recordNumber = -1;

float temperature;
bool contactIsOpen;

/// <summary>
///     Function to parse UART Rx messages and send to IoT Hub.
/// </summary>
/// <param name="msgToParse">The message received from the UART</param>
void parseAndSendToAzure(char *msgToParse)
{
    // This is a big ugly function, basically what it does is . . . 
    // 1. check to see if this is a advertisement message
    // 2. Pull the Address, recordNumber and flags from the message
    // 3. Check to see if we've already created an object for this device using the addres
    // 3.1 If not, then create one (just populate a static array)

    // Message pointer
    BT510Message_t *msgPtr;

    int tempBT510Index = -1;

    // Check to see if this is a BT510 Advertisement message
    if (strlen(msgToParse) > 32) {

        // Cast the message to the correct type so we can index into the string
        msgPtr = (BT510Message_t *)msgToParse;

        // Pull the BT510 address from the message
        getBdAddress(bdAddress, msgPtr);

        // Pull the record number.  The device sends the same message multiple times.  We
        // can use the record number to ignore duplicate messages

        uint16_t tempRecordNumber = (uint16_t)(stringToInt(&msgPtr->recordNumber[2], 2) << 8) |
                                    (uint16_t)(stringToInt(&msgPtr->recordNumber[0], 2) << 0);

        // Pull flags
        sensorFlags = (uint16_t)(stringToInt(&msgPtr->flags[2], 2) << 8) |
                      (uint16_t)(stringToInt(&msgPtr->flags[0], 2) << 0);

        // Determine if we know about this BT510 using the address
        currentBT510DeviceIndex = getBT510DeviceIndex(bdAddress);

        // Check to see if the device was found, not then add it!
        if (currentBT510DeviceIndex == -1) {

            // We did not find this device in our list, add it!
            Log_Debug("Add new device to list!\n");
            tempBT510Index = addBT510DeviceToList(bdAddress, msgPtr);

            if (tempBT510Index != -1) {

                currentBT510DeviceIndex = tempBT510Index;
            } else {

                // Device could not be added!
                Log_Debug("ERROR: Could not add new device\n");
            }
        }

        // Else the device was found and currentBT510DeviceIndex now holds the index to this
        // device's struct

        if (BT510DeviceList[currentBT510DeviceIndex].recordNumber == tempRecordNumber) {

            // We've seen this record already, print a message and bail!
            Log_Debug("Duplicate record #%d, from %s discarding message!\n", tempRecordNumber, bdAddress);

        } else // New record number, process it!
        {

// Assume we'll be sending a message to Azure and allocate a buffer
#define JSON_BUFFER_SIZE 128
            char telemetryBuffer[JSON_BUFFER_SIZE];

            // Capture the new record number in the array
            BT510DeviceList[currentBT510DeviceIndex].recordNumber = tempRecordNumber;

/*
            Log_Debug("Data Received from: ");
            // Determine if message was from original sender or repeater
            if (msgPtr->msgSendRxId[1] == 'S') {
                Log_Debug("Originating device\n");
            } else {
                Log_Debug("Repeater device\n");
            }
*/
            // Pull the device Name
            getDeviceName(deviceName, msgPtr);

            // Pull the Firmware Version
            getFirmwareVersion(firmwareVersion, msgPtr);

            // Pull the Bootloader Version
            getBootloaderVersion(bootloaderVersion, msgPtr);

            // Pull the rssi number from the end of the message
            getRxRssi(rxRssi, msgPtr);

            // Pull and output the data
            sensorData = (uint32_t)(stringToInt(&msgPtr->data[6], 2) << 24) |
                         (uint32_t)(stringToInt(&msgPtr->data[4], 2) << 16) |
                         (uint32_t)(stringToInt(&msgPtr->data[2], 2) << 8) |
                         (uint32_t)(stringToInt(&msgPtr->data[0], 2) << 0);

            parseFlags(sensorFlags);

            Log_Debug("\n\nBT510 Address: %s\n", bdAddress);
            Log_Debug("Device Name: %s is captured in index %d\n", deviceName,
                      currentBT510DeviceIndex);
            Log_Debug("Sensor Flags: 0x%04X\n", sensorFlags);
            Log_Debug("Record Number: %d\n", BT510DeviceList[currentBT510DeviceIndex].recordNumber);
            Log_Debug("Sensor Data: 0x%08X\n", sensorData);
            Log_Debug("Firmware Version: %s\n", firmwareVersion);
            Log_Debug("Bootloader Version: %s\n", bootloaderVersion);
            Log_Debug("RX rssi: %s\n", rxRssi);

            // Look at the record type to determine what to do next
            Log_Debug("Record Type: %d\n", stringToInt(msgPtr->recordType, 2));

            switch (stringToInt(msgPtr->recordType, 2)) {
            case RT_TEMPERATURE:
                temperature = (float)(((int16_t)(sensorData)) / 100.0);
                Log_Debug("T_TEMPERATURE: Reported Temperature: %.2fC\n", temperature);

                snprintf(telemetryBuffer, sizeof(telemetryBuffer), bt510TelemetryJsonObject,
                         deviceName, bdAddress, rxRssi, temperature);
                Log_Debug("TX: %s\n", telemetryBuffer);

                // Send the telemetry message
                SendTelemetry(telemetryBuffer, NULL, NULL);

                break;
            case RT_MAGNET:
                Log_Debug("RT_MAGNET\n");
                BT510DeviceList[currentBT510DeviceIndex].lastContactIsOpen =
                    (sensorFlags >> FLAG_MAGNET_STATE) & 1U;
                break;
            case RT_MOVEMENT:
                Log_Debug("RT_MOVEMENT\n");
                break;
            case RT_ALARM_HIGH_TEMP1:
                Log_Debug("RT_ALARM_HIGH_TEMP1\n");
                Log_Debug("Reported Temperature: %.2fC\n",
                          (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_HIGH_TEMP2:
                Log_Debug("RT_ALARM_HIGH_TEMP2\n");
                Log_Debug("Reported Temperature: %.2fC\n",
                          (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_HIGH_TEMP_CLEAR:
                Log_Debug("RT_ALARM_HIGH_TEMP_CLEAR\n");
                Log_Debug("Reported Temperature: %.2fC\n",
                          (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_LOW_TEMP1:
                Log_Debug("RT_ALARM_LOW_TEMP1\n");
                Log_Debug("Reported Temperature: %.2fC\n",
                          (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_LOW_TEMP2:
                Log_Debug("RT_ALARM_LOW_TEMP2\n");
                Log_Debug("Reported Temperature: %.2fC\n",
                          (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_LOW_TEMP_CLEAR:
                Log_Debug("RT_ALARM_LOW_TEMP_CLEAR\n");
                Log_Debug("Reported Temperature: %.2f\n", (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_ALARM_DELTA_TEMP:
                Log_Debug("RT_ALARM_DELTA_TEMP\n");
                Log_Debug("Reported Temperature: %.2f\n", (float)(((int16_t)(sensorData)) / 100.0));
                break;
            case RT_BATTERY_GOOD:
                Log_Debug("RT_BATTERY_GOOD\n");
                Log_Debug("Reported Voltage: %dmV\n", sensorData);
                break;
            case RT_BATTERY_BAD:
                Log_Debug("RT_BATTERY_BAD\n");
                Log_Debug("Reported Voltage: %dmV\n", sensorData);
                break;
            case RT_ADVERTISE_ON_BUTTON:
                Log_Debug("RT_ADVERTISE_ON_BUTTON\n");
                break;
            case RT_RESET:
                Log_Debug("RT_RESET: Reason %d\n", sensorData);

                break;
            case RT_RESERVED0:
            case RT_RESERVED1:
            case RT_RESERVED2:
                Log_Debug("RT_RESERVED\n");
                break;
            case RT_SKIP_A_ENUM:
            default:
                Log_Debug("Unknown record type!\n");
            }
        }
    }
}

int stringToInt(char *stringData, size_t stringLength)
{

    char tempString[64];
    strncpy(tempString, stringData, stringLength);
    tempString[stringLength] = '\0';
    return (int)(strtol(tempString, NULL, 16));
}

void textFromHexString(char *hex, char *result, int strLength)
{
    char temp[3];

    temp[2] = '\0';
    for (int i = 0; i < strLength; i += 2) {
        strncpy(temp, &hex[i], 2);
        *result = (char)strtol(temp, NULL, 16);
        result++;
    }
    *result = '\0';
}

// Pull the device Name
void getDeviceName(char *outputString, BT510Message_t *rxMessage)
{

    // Pull and validate device name length
    if (MAX_NAME_LENGTH >= stringToInt(rxMessage->deviceNameLength, 2)) {
        textFromHexString(rxMessage->deviceNameString, deviceName,
                          stringToInt(rxMessage->deviceNameLength, 2) * 2);
    } else {
        Log_Debug("Name is greater than MAX length!\n");
    }
}

// Set the global BT510 address variable
void getBdAddress(char *bdAddress, BT510Message_t *rxMessage)
{
    bdAddress[0] = rxMessage->BdAddress[10];
    bdAddress[1] = rxMessage->BdAddress[11];
    bdAddress[3] = rxMessage->BdAddress[8];
    bdAddress[4] = rxMessage->BdAddress[9];
    bdAddress[6] = rxMessage->BdAddress[6];
    bdAddress[7] = rxMessage->BdAddress[7];
    bdAddress[9] = rxMessage->BdAddress[4];
    bdAddress[10] = rxMessage->BdAddress[5];
    bdAddress[12] = rxMessage->BdAddress[2];
    bdAddress[13] = rxMessage->BdAddress[3];
    bdAddress[15] = rxMessage->BdAddress[0];
    bdAddress[16] = rxMessage->BdAddress[1];
}

// Set the global firmware version variable
void getFirmwareVersion(char *firmwareVersion, BT510Message_t *rxMessage)
{
    firmwareVersion[0] = rxMessage->firmwareVersion[0];
    firmwareVersion[1] = rxMessage->firmwareVersion[1];
    firmwareVersion[3] = rxMessage->firmwareVersion[2];
    firmwareVersion[4] = rxMessage->firmwareVersion[3];
    firmwareVersion[6] = rxMessage->firmwareVersion[4];
    firmwareVersion[7] = rxMessage->firmwareVersion[5];
}

// Set the global boot loader version variable
void getBootloaderVersion(char *bootloaderVersion, BT510Message_t *rxMessage)
{
    bootloaderVersion[0] = rxMessage->bootLoaderVersion[0];
    bootloaderVersion[1] = rxMessage->bootLoaderVersion[1];
    bootloaderVersion[3] = rxMessage->bootLoaderVersion[2];
    bootloaderVersion[4] = rxMessage->bootLoaderVersion[3];
    bootloaderVersion[6] = rxMessage->bootLoaderVersion[4];
    bootloaderVersion[7] = rxMessage->bootLoaderVersion[5];

}

// Set the global rssi variable from the end of the message
void getRxRssi(char *rxRssi, BT510Message_t *rxMessage)
{
    // Pull the last three characters from the incomming message.  Use the deviceNameString as a
    // starting point then take the next three characters.
    rxRssi[0] = (rxMessage->deviceNameString[stringToInt(rxMessage->deviceNameLength, 2) * 2 + 1]);
    rxRssi[1] = (rxMessage->deviceNameString[stringToInt(rxMessage->deviceNameLength, 2) * 2 + 2]);
    rxRssi[2] = (rxMessage->deviceNameString[stringToInt(rxMessage->deviceNameLength, 2) * 2 + 3]);
}

void parseFlags(uint16_t flags)
{
    for (int i = 0; i < 16; i++) {

        if ((int)flags >> i & 1) {

            switch (i) {
            case FLAG_RTC_SET:
                //                Log_Debug("RTC_SET flag set\n");
                break;
            case FLAG_ACTIVITY_MODE:
                //                Log_Debug("ACTIVITY_MODE flag set\n");
                break;
            case FLAG_ANY_FLAG_WAS_SET:
                //                Log_Debug("ANY_FLAG_WAS_SET flag set\n");
                break;
            case FLAG_LOW_BATTERY_ALARM:
                //                Log_Debug("LOW_BATTERY_ALARM flag set\n");
                break;
            case FLAG_HIGH_TEMP_ALARM_BIT0:
                //                Log_Debug("HIGH_TEMP_ALARM_BIT0 flag set\n");
                break;
            case FLAG_HIGH_TEMP_ALARM_BIT1:
                //                Log_Debug("HIGH_TEMP_ALARM_BIT1 flag set\n");
                break;
            case FLAG_LOW_TEMP_ALARM_BIT0:
                //                Log_Debug("LOW_TEMP_ALARM_BIT0 flag set\n");
                break;
            case FLAG_LOW_TEMP_ALARM_BIT1:
                //                Log_Debug("LOW_TEMP_ALARM_BIT1 flag set\n");
                break;
            case FLAG_DELTA_TEMP_ALARM:
                //                Log_Debug("DELTA_TEMP_ALARM\n");
                break;
            case FLAG_MOVEMENT_ALARM:
                //                Log_Debug("MOVEMENT_ALARM flag set\n");
                break;
            case FLAG_MAGNET_STATE:
                //                Log_Debug("MAGNET_STATE flag set\n");
                break;
            case FLAG_RESERVED0:
            case FLAG_RESERVED1:
            case FLAG_RESERVED2:
            case FLAG_RESERVED3:
            case FLAG_RESERVED4:
                //                Log_Debug("Reserved flag set?\n");
            default:
                break;
            }
        }
    }
}

int getBT510DeviceIndex(char *BT510DeviceID)
{

    for (int8_t i = 0; i < numBT510DevicesInList; i++) {
        if (strncmp(BT510DeviceList[i].bdAddress, BT510DeviceID, strlen(BT510DeviceID)) == 0) {
            return i;
        }
    }

    // If we did not find the device return -1
    return -1;
}
int addBT510DeviceToList(char *newBT510Address, BT510Message_t *newBT510Device)
{

    // Check to make sure the list is not already full, if so return -1 (failure)
    if (numBT510DevicesInList == 10) {
        return -1;
    }
    // Increment the number of devices in the list, then fill in the new slot
    numBT510DevicesInList++;

    // Define the return value as the index into the array for the new element
    int newDeviceIndex = numBT510DevicesInList - 1;

    BT510DeviceList[newDeviceIndex].recordNumber = 0;
    BT510DeviceList[newDeviceIndex].lastContactIsOpen = (sensorFlags > FLAG_MAGNET_STATE) & 1U;
    strncpy(BT510DeviceList[newDeviceIndex].bdAddress, newBT510Address, strlen(newBT510Address));

    // Return the index into the array where we added the new device
    return newDeviceIndex;
}
