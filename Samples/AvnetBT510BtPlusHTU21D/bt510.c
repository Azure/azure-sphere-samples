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

//#define ENABLE_MESSAGE_TESTING
//#define ENABLE_MSG_DEBUG

#include "bt510.h"
#include "math.h"
#include "htu21d.h"

// Send the telemetry message
#ifdef USE_IOT_CONNECT
#include "iotConnect.h"
#endif

// Global variables
BT510Device_t BT510DeviceList[MAX_BT510_DEVICES];
char authorizedDeviceList[MAX_BT510_DEVICES][BT510_ADDRESS_LEN];
int currentBT510DeviceIndex = -1;
uint8_t numBT510DevicesInList = 0;
char deviceName[MAX_NAME_LENGTH + 1];
char bdAddress[] = "  -  -  -  -  -  \0";
char firmwareVersion[] = "  .  .  \0";
char bootloaderVersion[] = "  .  .  \0";
char rxRssi[] = "-xx\0";
uint32_t sensorData;
uint16_t sensorFlags;

// Variable to hold the local (htu21d) sensor readings
float htu21dTemperature = 0.0;
float htu21dHumidity = 0.0;



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
    // 3. Make sure that the mac for this device was authorized in the device twin
    // 4. Check to see if we've already created an object for this device using the addres
    // 4.1 If not, then create one (just populate a static array)

    // Message pointer
    BT510Message_t *msgPtr;

    int tempBT510Index = -1;

    // Check to see if this is a BT510 Advertisement message
    if (strlen(msgToParse) > 32) {

        // Cast the message to the correct type so we can index into the string
        msgPtr = (BT510Message_t *)msgToParse;

            // Pull the device Name
        getDeviceName(deviceName, msgPtr);

        // Pull the Firmware Version
        getFirmwareVersion(firmwareVersion, msgPtr);

        // Pull the Bootloader Version
        getBootloaderVersion(bootloaderVersion, msgPtr);

        // Pull the BT510 address from the message
        getBdAddress(bdAddress, msgPtr);

        // Check to see if this devcice's MAC address has been whitelisted
        if( !isDeviceAuthorized(bdAddress)){

            Log_Debug("Device %s has not been Authorized, discarding message data\n", bdAddress);
            Log_Debug("To authorize the device add it's MAC address as a authorizedMaxX in the IoTHub device twin\n");
            return;

        }

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
            tempBT510Index = addBT510DeviceToList(bdAddress, msgPtr);

            if (tempBT510Index != -1) {

                currentBT510DeviceIndex = tempBT510Index;
                Log_Debug("Add this device as index %d\n", currentBT510DeviceIndex);
            } else {

                // Device could not be added!
                Log_Debug("ERROR: Could not add new device\n");
            }
        }

        // Else the device was found and currentBT510DeviceIndex now holds the index to this
        // device's struct

#ifdef ENABLE_MESSAGE_TESTING

        // If we're testing messages, then don't check for duplicate messages, parse them no matter what
        if (false) {
#else
        // Check to see if we're already processed this record number for this device, if so then don't 
        // process it again
        if (BT510DeviceList[currentBT510DeviceIndex].recordNumber == tempRecordNumber) {
#endif 
#ifdef ENABLE_MSG_DEBUG
            // We've seen this record already, print a message and bail!
            Log_Debug("Duplicate record #%d, from %s discarding message!\n", tempRecordNumber, bdAddress);
#endif

        } else // New record number, process it!
        {

            // Capture the new record number in the array
            BT510DeviceList[currentBT510DeviceIndex].recordNumber = tempRecordNumber;
#ifdef ENABLE_MSG_DEBUG
            Log_Debug("Data Received from: ");
            // Determine if message was from original sender or repeater
            if (msgPtr->msgSendRxId[1] == 'S') {
                Log_Debug("Originating device\n");
            } else {
                Log_Debug("Repeater device\n");
            }
#endif 
            // Pull the rssi number from the end of the message
            getRxRssi(rxRssi, msgPtr);
            BT510DeviceList[currentBT510DeviceIndex].lastRssi = atoi(rxRssi);

            // Pull the data from the message
            sensorData = (uint32_t)(stringToInt(&msgPtr->data[6], 2) << 24) |
                         (uint32_t)(stringToInt(&msgPtr->data[4], 2) << 16) |
                         (uint32_t)(stringToInt(&msgPtr->data[2], 2) << 8) |
                         (uint32_t)(stringToInt(&msgPtr->data[0], 2) << 0);

            parseFlags(sensorFlags);

#ifdef ENABLE_MSG_DEBUG
            Log_Debug("\n\nBT510 Address: %s\n", bdAddress);
            Log_Debug("Device Name: %s is captured in index %d\n", deviceName,
                      currentBT510DeviceIndex);
            Log_Debug("Sensor Flags: 0x%04X\n", sensorFlags);
            Log_Debug("Record Number: %d\n", BT510DeviceList[currentBT510DeviceIndex].recordNumber);
            Log_Debug("Sensor Data: 0x%08X\n", sensorData);
            Log_Debug("Firmware Version: %s\n", firmwareVersion);
            Log_Debug("Bootloader Version: %s\n", bootloaderVersion);
            Log_Debug("RX rssi: %s\n", rxRssi);
#endif 
            processData(stringToInt(msgPtr->recordType, 2), currentBT510DeviceIndex);

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

    // check the whitelist first!
    if( !isDeviceAuthorized(bdAddress)){
        Log_Debug("Device not authorized, not adding to list\n");
        return -1;
    }

    Log_Debug("Device IS authorized\n");

    // Check to make sure the list is not already full, if so return -1 (failure)
    if (numBT510DevicesInList == 10) {
        return -1;
    }
    // Increment the number of devices in the list, then fill in the new slot
    numBT510DevicesInList++;

    // Define the return value as the index into the array for the new element
    int newDeviceIndex = numBT510DevicesInList - 1;

    // Update the structure for this device
    BT510DeviceList[newDeviceIndex].recordNumber = 0;
    BT510DeviceList[newDeviceIndex].lastContactIsOpen = (sensorFlags > FLAG_MAGNET_STATE) & 1U;
    strncpy(BT510DeviceList[newDeviceIndex].bdAddress, newBT510Address, strlen(newBT510Address));
    strncpy(BT510DeviceList[newDeviceIndex].bt510Name, deviceName, strlen(deviceName));
    BT510DeviceList[newDeviceIndex].lastTemperature = NAN;
    BT510DeviceList[newDeviceIndex].lastBattery = NAN;

    // Send up this devices specific details to the device twin
    #define JSON_TWIN_BUFFER_SIZE 512
    char deviceTwinBuffer[JSON_TWIN_BUFFER_SIZE];

    snprintf(deviceTwinBuffer, sizeof(deviceTwinBuffer), bt510DeviceTwinsonObject, deviceName, deviceName, deviceName, bdAddress, deviceName,
                 firmwareVersion, deviceName, bootloaderVersion);
    TwinReportState(deviceTwinBuffer);

    Log_Debug("Add new device to list at index %d!\n", newDeviceIndex);

    // Return the index into the array where we added the new device
    return newDeviceIndex;
}


// Process the record.  For most record types, we just capture the data into variables and wait for the
// send Telemetry event to fire; we'll send the latest telemetry then.  
// For alarms, or events, we send the telemetry right away in case we need to take action on the event(s)
void processData(int recordType, int deviceIndex) {

    bool sendTelemetryNow = false;

    // Look at the record type to determine what to do next
    Log_Debug("Record Type: %d\n", recordType);

    // Assume we'll be sending a message to Azure and allocate a buffer
    #define JSON_BUFFER_SIZE 256
    char telemetryBuffer[JSON_BUFFER_SIZE];

    switch (recordType)
    {
    case RT_ALARM_HIGH_TEMP1:
    case RT_ALARM_HIGH_TEMP2:
    case RT_ALARM_HIGH_TEMP_CLEAR:
    case RT_ALARM_LOW_TEMP1:
    case RT_ALARM_LOW_TEMP2:
    case RT_ALARM_LOW_TEMP_CLEAR:
    case RT_ALARM_DELTA_TEMP:

    // For any of the "Alarm cases", set the flag to send telemetry now!
    sendTelemetryNow = true;

    // Fall into the Temperture case to update the variable
    case RT_TEMPERATURE:
//        Log_Debug("New Temperature case: %d\n", recordType);
        temperature = (float)(((int16_t)(sensorData)) / 100.0);
        BT510DeviceList[deviceIndex].lastTemperature = temperature;
        Log_Debug("Reported Temperature: %.2f\n", temperature);
        break;
    case RT_BATTERY_GOOD:
//        Log_Debug("\nRT_BATTERY_GOOD\n");
        Log_Debug("Reported Voltage: %2.3fV\n", (float)sensorData/1000);
        BT510DeviceList[deviceIndex].lastBattery = (float)sensorData/1000;
        break;
    case RT_BATTERY_BAD:
//        Log_Debug("\nRT_BATTERY_BAD\n");
        Log_Debug("Reported Voltage: %2.3fV\n", (float)sensorData / 1000);
        BT510DeviceList[deviceIndex].lastBattery = (float)sensorData/1000;
        // Send the telemetry now!
        sendTelemetryNow = true;
        break;
    case RT_MAGNET:
//        Log_Debug("\nRT_MAGNET is %s\n", ((sensorFlags >> FLAG_MAGNET_STATE) & 1U) ? "Far": "Near");
        
        BT510DeviceList[currentBT510DeviceIndex].lastContactIsOpen =
            (sensorFlags >> FLAG_MAGNET_STATE) & 1U;

        BT510DeviceList[currentBT510DeviceIndex].lastContactIsOpen = (sensorFlags >> FLAG_MAGNET_STATE) & 1U;
        snprintf(telemetryBuffer, sizeof(telemetryBuffer), bt510MagnetTelemetryJsonObject, 
                    BT510DeviceList[deviceIndex].bt510Name, (sensorFlags >> FLAG_MAGNET_STATE) & 1U);

        // Send the telemetry message
        SendTelemetry(telemetryBuffer, true);

    case RT_MOVEMENT:
//        Log_Debug("\nRT_MOVEMENT\n");

        snprintf(telemetryBuffer, sizeof(telemetryBuffer), bt510MovementTelemetryJsonObject, BT510DeviceList[deviceIndex].bt510Name);

        // Send the telemetry message
        SendTelemetry(telemetryBuffer, true);

        break;

    case RT_ADVERTISE_ON_BUTTON:
//        Log_Debug("\nRT_ADVERTISE_ON_BUTTON\n");
        Log_Debug("Reported Voltage: %2.3fV\n", (float)sensorData / 1000);
        BT510DeviceList[deviceIndex].lastBattery = (float)sensorData/1000;

        // Send the telemetry now!
        sendTelemetryNow = true;
        break;
    case RT_RESET:
//        Log_Debug("\nRT_RESET: Reason %d\n", sensorData);
        snprintf(telemetryBuffer, sizeof(telemetryBuffer), bt510ResetAlarmTelemetryJsonObject,
                 BT510DeviceList[deviceIndex].bt510Name, sensorData);

        // Send the telemetry message
        SendTelemetry(telemetryBuffer, true);
        break;
    case RT_RESERVED0:
    case RT_RESERVED1:
    case RT_RESERVED2:
//        Log_Debug("\nRT_RESERVED\n");
        break;
    case RT_SKIP_A_ENUM:
    default:
        
//        Log_Debug("Unknown record type!\n");
        break; 
    }

    // If we need to send the telemetry data up now, then do it.
    if(sendTelemetryNow){
        bt510SendTelemetry();
    }
}

// Check to see if the devices MAC has been authorized
bool isDeviceAuthorized(char* deviceToCheck){

    for(int i = 0; i < MAX_BT510_DEVICES; i++){
        
        if(strncmp(&authorizedDeviceList[i][0], deviceToCheck, BT510_ADDRESS_LEN) == 0){
            return true;
        }
    }
    return false;
}

void bt510SendTelemetry(){

    // Define a flag to see if we found new telemetry data to send
    bool updatedValuesFound = false;

    // Define a flag to use for each device to determine if we need to append the rssi telemetry
    bool deviceWasUpdated = false;

    // Dynamically build the telemetry JSON document to handle from 1 to 10 BT510 devices.  
    // Telemetry will always have the htu21d temperature + humidity telemetry at the beginning
    // For example the JSON for 1 BT510 . . . 
    // {"temp": 22.4, "humidity: 23.3, "tempDev1":24.3, "batDev1": 3.23, "rssiDev1": -71}
    //
    // The JSON for two BT510s . . .
    // {"temp": 22.4, "humidity: 23.3,"tempDev1":24.3, "batDev1": 3.23, "tempDev2":24.3, "batDev2": 3.23, "rssiDev1": -70, , "rssiDev2": -65}
    // Where Dev1 and Dev2 are dynamic names pulled from each BT510s advertised name
    
    // If we don't have any devices in the list, then bail.  Nothing to see here, move along . . . 
    if(numBT510DevicesInList == 0){
        return;
    }

    // Read the HTU21D sensor before sending telemetry
    if (htu21d_read_temperature_and_relative_humidity(&htu21dTemperature, &htu21dHumidity) ==
        htu21d_status_ok) {
    
        Log_Debug("Htu21D: Temp: %.2f, Humidity %.2f\n", htu21dTemperature, htu21dHumidity);

    } else 
    {
        Log_Debug("Error reading HTU21D sensor!\n");
    }

    // Allocate enough memory to hold the dynamic JSON document, we know how large the object is, but we need to add additional
    // memory for the device name and the data that we'll be adding to the telemetry message
    char *telemetryBuffer = calloc((size_t)(numBT510DevicesInList * (               // Multiply the size for one device by the number of devices we have
                                   (size_t)strlen(htu21dTempHumidityJsonObject) +   // Size of the htu21d json template
                                   (size_t)strlen(bt510TemperatureJsonObject) +     // Size of the temperature json template
                                   (size_t)strlen(bt510BatteryJsonObject) +         // Size of the battery json template
                                   (size_t)strlen(bt510RssiJsonObject) +            // Size of the rssi json template
                                   (size_t)32 +                                     // Allow for the temperature and battery data (the numbers)
                                   (size_t)MAX_NAME_LENGTH)),                       // Allow for the device name i.e., "Basement + Coach"
                                   sizeof(char));

    // Verify we got the memory requested
    if(telemetryBuffer == NULL){
        exitCode = ExitCode_Init_TelemetryCallocFailed;
        return;
    }

    // Declare an array that we use to construct each of the different telemetry parts i.e., "temperature, 23.22"
    char newTelemetryString[strlen(htu21dTempHumidityJsonObject) + 16 + MAX_NAME_LENGTH];

    // Start to build the dynamic telemetry message.  This first string contains the opening '{'
    snprintf(newTelemetryString, sizeof(newTelemetryString), htu21dTempHumidityJsonObject, 
                                                             htu21dTemperature, 
                                                             htu21dHumidity);
    // Add it to the telemetry message
    strcat(telemetryBuffer,newTelemetryString);

    for(int i = 0; i < numBT510DevicesInList; i++){
        
        // Add temperature data for the current device, if it's been updated
        if(!isnan(BT510DeviceList[i].lastTemperature)){

            snprintf(newTelemetryString, sizeof(newTelemetryString), bt510TemperatureJsonObject, 
                                                                     BT510DeviceList[i].bt510Name, 
                                                                     BT510DeviceList[i].lastTemperature);
            // Add it to the telemetry message
            strcat(telemetryBuffer,newTelemetryString);

            // Mark the temperature variable with the NAN value so we can determine if it gets updated
            BT510DeviceList[i].lastTemperature = NAN;

            // Set the flag that tells the logic to send the message to Azure
            updatedValuesFound = true;
            deviceWasUpdated = true;

        }

        // Add battery data for the current device, if it's been updated
        if(!isnan(BT510DeviceList[i].lastBattery)){

            snprintf(newTelemetryString, sizeof(newTelemetryString), bt510BatteryJsonObject, 
                                                                     BT510DeviceList[i].bt510Name, 
                                                                     BT510DeviceList[i].lastBattery);
            // Add it to the telemetry message
            strcat(telemetryBuffer,newTelemetryString);

            // Mark the battery variable with the NAN value so we can determine if it gets updated
            BT510DeviceList[i].lastBattery = NAN;

            // Set the flag that tells the logic to send the message to Azure
            updatedValuesFound = true;
            deviceWasUpdated = true;

        }

        // If we found updated values to send, then tack on the current rssi reading
        if(deviceWasUpdated){

            snprintf(newTelemetryString, sizeof(newTelemetryString), bt510RssiJsonObject, 
                                                                     BT510DeviceList[i].bt510Name, 
                                                                     BT510DeviceList[i].lastRssi);
            // Add it to the telemetry message
            strcat(telemetryBuffer,newTelemetryString);
            
            // Clear the flag, we only want to send the rssi if we've received an update, otherwise it's old data
            deviceWasUpdated = false;
        }
    }

    // Find the last location of the constructed string (will contain the last ',' char), and overwrite it with a closing '}'
    telemetryBuffer[strlen(telemetryBuffer)-1] = '}';
    
    // Null terminate the string
    telemetryBuffer[strlen(telemetryBuffer)] = '\0';

    if(updatedValuesFound){
        
        Log_Debug("Telemetry message: %s\n", telemetryBuffer);
        // Send the telemetry message
        SendTelemetry(telemetryBuffer, true);
    }
    else{
        Log_Debug("No new data found, not sending telemetry update\n");
    }

    free(telemetryBuffer);

}