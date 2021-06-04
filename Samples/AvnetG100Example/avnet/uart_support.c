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

#include "uart_support.h"

// File descriptors - initialized to invalid value
static int uartFd = -1;

static EventRegistration *uartEventReg = NULL;

#ifdef ENABLE_DEBUG_TO_UART
// Global variable to controll sending debug messages to the UART
bool sendDebug = ENABLE_DEBUG_BY_DEFAULT;
#endif 

/// <summary>
///     Helper function to send a fixed message via the UART
/// </summary>
/// <param name="dataToSend">The data to send over the UART</param>
void SendUartMessage(const char *dataToSend)
{
    size_t totalBytesSent = 0;
    size_t totalBytesToSend = strlen(dataToSend);
    int sendIterations = 0;
    
    // As long as there are more bytes to send
    while (totalBytesSent < totalBytesToSend) {
        sendIterations++;

        // Send as much of the remaining data as possible
        size_t bytesLeftToSend = totalBytesToSend - totalBytesSent;
        const char *remainingMessageToSend = dataToSend + totalBytesSent;
        size_t bytesSent = write(uartFd, remainingMessageToSend, bytesLeftToSend);
        if (bytesSent == -1) {
            Log_Debug("ERROR: Could not write to UART: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_SendMessage_Write;
            return;
        }

        totalBytesSent += (size_t)bytesSent;
    }

    Log_Debug("Sent %zu bytes over UART in %d calls.\n", totalBytesSent, sendIterations);
}

/// <summary>
///     Handle UART event: if there is incoming data, print it.
///     This satisfies the EventLoopIoCallback signature.
/// </summary>
static void UartEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    // Uncomment for circular queue debug
    //#define ENABLE_UART_DEBUG

#define RX_BUFFER_SIZE 128
#define DATA_BUFFER_SIZE 128
#define DATA_BUFFER_MASK (DATA_BUFFER_SIZE - 1)

    // Buffer for incomming data
    uint8_t receiveBuffer[RX_BUFFER_SIZE];

    // Buffer for persistant data.  Sometimes we don't receive all the
    // data at once so we need to store it in a persistant buffer before processing.
    static uint8_t dataBuffer[DATA_BUFFER_SIZE];

    // The index into the dataBuffer to write the next piece of RX data
    static int nextData = 0;

    // The index to the head of the valid/current data, this is the beginning
    // of the next response
    static int currentData = 0;

    // The number of btyes in the dataBuffer, used to make sure we don't overflow the buffer
    static int bytesInBuffer = 0;

    ssize_t bytesRead;

    // Read the uart
    bytesRead = read(uartFd, receiveBuffer, RX_BUFFER_SIZE);

#ifdef ENABLE_UART_DEBUG
    Log_Debug("Enter: bytesInBuffer: %d\n", bytesInBuffer);
    Log_Debug("Enter: bytesRead: %d\n", bytesRead);
    Log_Debug("Enter: nextData: %d\n", nextData);
    Log_Debug("Enter: currentData: %d\n", currentData);
#endif

    // Check to make sure we're not going to over run the buffer
    if ((bytesInBuffer + bytesRead) > DATA_BUFFER_SIZE) {

        // The buffer is full, attempt to recover by emptying the buffer!
        Log_Debug("Buffer Full!  Purging\n");

        nextData = 0;
        currentData = 0;
        bytesInBuffer = 0;
        return;
    }

    // Move data from the receive Buffer into the Data Buffer.  We do this
    // because sometimes we don't receive the entire message in one uart read.
    for (int i = 0; i < bytesRead; i++) {

        // Copy the data into the dataBuffer
        dataBuffer[nextData] = receiveBuffer[i];
#ifdef ENABLE_UART_DEBUG
        Log_Debug("dataBuffer[%d] = %c\n", nextData, receiveBuffer[i]);
#endif
        // Increment the bytes count
        bytesInBuffer++;

        // Increment the nextData pointer and adjust for wrap around
        nextData = ((nextData + 1) & DATA_BUFFER_MASK);
    }

    // Check to see if we can find a response.  A response will end with a '\n' character
    // Start looking at the beginning of the first non-processed message @ currentData

    // Use a temp buffer pointer in case we don't find a message
    int tempCurrentData = currentData;

    // Iterate over the valid data from currentData to nextData locations in the buffer
    while (tempCurrentData != nextData) {
        if (dataBuffer[tempCurrentData] == '\n') {

#ifdef ENABLE_UART_DEBUG
            // Found a message from index currentData to tempNextData
            Log_Debug("Found message from %d to %d\n", currentData, tempCurrentData);
#endif
            // Determine the size of the new message we just found, account for the case
            // where the message wraps from the end of the buffer to the beginning
            int responseMsgSize = 0;
            if (currentData > tempCurrentData) {
                responseMsgSize = (DATA_BUFFER_SIZE - currentData) + tempCurrentData;
            } else {
                responseMsgSize = tempCurrentData - currentData;
            }

            // Declare a new buffer to hold the response we just found
            uint8_t responseMsg[responseMsgSize + 1];

            // Copy the response from the buffer, do it one byte at a time
            // since the message may wrap in the data buffer
            for (int j = 0; j < responseMsgSize; j++) {
                responseMsg[j] = dataBuffer[(currentData + j) & DATA_BUFFER_MASK];
                bytesInBuffer--;
            }

            // Decrement the bytesInBuffer one more time to account for the '\n' charcter
            bytesInBuffer--;

            // Null terminate the message and print it out to debug
            responseMsg[responseMsgSize] = '\0';
            Log_Debug("RX: %s\n", responseMsg);

            // Call the routine that knows how to parse the response and send data to Azure
//            parseAndSendToAzure(responseMsg);

            // Update the currentData index and adjust for the '\n' character
            currentData = tempCurrentData + 1;
            // Overwrite the '\n' character so we don't accidently find it and think
            // we found a new mssage
            dataBuffer[tempCurrentData] = '\0';
        }

        else if (tempCurrentData == nextData) {

#ifdef ENABLE_UART_DEBUG
            Log_Debug("No message found, exiting . . . \n");
#endif
            return;
        }

        // Increment the temp CurrentData pointer and let it wrap if needed
        tempCurrentData = ((tempCurrentData + 1) & DATA_BUFFER_MASK);
    }

#ifdef ENABLE_UART_DEBUG
    Log_Debug("Exit: nextData: %d\n", nextData);
    Log_Debug("Exit: currentData: %d\n", currentData);
    Log_Debug("Exit: bytesInBuffer: %d\n", bytesInBuffer);
#endif
}
/// <summary>
///     Initialize the UART and the UART event handler
/// </summary>
ExitCode initUart(void){

    // Create a UART_Config object, open the UART and set up UART event handler
    UART_Config uartConfig;
    UART_InitConfig(&uartConfig);
    uartConfig.baudRate = 115200;
    uartConfig.flowControl = UART_FlowControl_RTSCTS;
    uartConfig.dataBits = UART_DataBits_Eight;
    uartConfig.parity = UART_Parity_None;
    uartConfig.stopBits = UART_StopBits_One;
    uartFd = UART_Open(EXTERNAL_UART, &uartConfig);
    if (uartFd == -1) {
        Log_Debug("ERROR: Could not open UART: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_UartOpen;
    }
    uartEventReg = EventLoop_RegisterIo(eventLoop, uartFd, EventLoop_Input, UartEventHandler, NULL);
    if (uartEventReg == NULL) {
        return ExitCode_Init_RegisterIo;
    }
    return ExitCode_Success;
}

/// <summary>
///     Close the UART file descriptor
/// </summary>
void CloseUart(void){
    
    if (uartFd >= 0) {
        int result = close(uartFd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", "UART", strerror(errno), errno);
        }
    }
}