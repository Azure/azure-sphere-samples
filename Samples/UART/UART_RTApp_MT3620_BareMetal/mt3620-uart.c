/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include "mt3620-baremetal.h"
#include "mt3620-uart.h"

// This is the physical TX FIFO size, taken from the datasheet.
// To adjust the size of the in-memory FIFO, set TX_BUFFER_SIZE below.
#define TX_FIFO_DEPTH 16

// This must be able to hold a value which is strictly greater than TX_BUFFER_SIZE.
typedef uint16_t EnqCtrType;

// Buffer sizes must be a power of two, and less than 65536.
#define TX_BUFFER_SIZE 256
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)
#define RX_BUFFER_SIZE 32
#define RX_BUFFER_MASK (RX_BUFFER_SIZE - 1)

typedef struct {
    uintptr_t baseAddr;
    int nvicIrq;
    uint8_t txBuffer[TX_BUFFER_SIZE];
    volatile EnqCtrType txEnqueuedBytes;
    volatile EnqCtrType txDequeuedBytes;

    Callback rxCallback;
    uint8_t rxBuffer[RX_BUFFER_SIZE];
    volatile EnqCtrType rxEnqueuedBytes;
    volatile EnqCtrType rxDequeuedBytes;
} UartInfo;

static UartInfo uarts[] = {
    [UartCM4Debug] = {.baseAddr = 0x21040000, .nvicIrq = 4},
    [UartIsu0] = {.baseAddr = 0x38070500, .nvicIrq = 47},
};

static void Uart_HandleIrq(UartId id);

void Uart_Init(UartId id, Callback rxCallback)
{
    UartInfo *unit = &uarts[id];

    // Configure UART to use 115200-8-N-1.
    WriteReg32(unit->baseAddr, 0x0C, 0xBF); // LCR (enable DLL, DLM)
    WriteReg32(unit->baseAddr, 0x08, 0x10); // EFR (enable enhancement features)
    WriteReg32(unit->baseAddr, 0x24, 0x3);  // HIGHSPEED
    WriteReg32(unit->baseAddr, 0x04, 0);    // Divisor Latch (MS)
    WriteReg32(unit->baseAddr, 0x00, 1);    // Divisor Latch (LS)
    WriteReg32(unit->baseAddr, 0x28, 224);  // SAMPLE_COUNT
    WriteReg32(unit->baseAddr, 0x2C, 110);  // SAMPLE_POINT
    WriteReg32(unit->baseAddr, 0x58, 0);    // FRACDIV_M
    WriteReg32(unit->baseAddr, 0x54, 223);  // FRACDIV_L
    WriteReg32(unit->baseAddr, 0x0C, 0x03); // LCR (8-bit word length)

    // FCR[RFTL] = 2 -> 12 element RX FIFO trigger
    // FCR[TFTL] = 1 -> 4 element TX FIFO trigger
    // FCR[CLRT] = 1 -> Clear Transmit FIFO
    // FCR[CLRR] = 1 -> Clear Receive FIFO
    // FCR[FIFOE] = 1 -> FIFO Enable
    const uint8_t fcr = (2U << 6) | (1U << 4) | (1U << 2) | (1U << 1) | (1U << 0);
    WriteReg32(unit->baseAddr, 0x08, fcr);

    // If an RX callback was supplied then enable the Receive Buffer Full Interrupt.
    if (rxCallback) {
        uarts[id].rxCallback = rxCallback;
        // IER[ERBGI] = 1 -> Enable Receiver Buffer Full Interrupt
        SetReg32(unit->baseAddr, 0x04, 0x01);
    }

    SetNvicPriority(unit->nvicIrq, UART_PRIORITY);
    EnableNvicInterrupt(unit->nvicIrq);
}

void Uart_HandleIrq4(void)
{
    Uart_HandleIrq(UartCM4Debug);
}

void Uart_HandleIrq47(void)
{
    Uart_HandleIrq(UartIsu0);
}

static void Uart_HandleIrq(UartId id)
{
    UartInfo *unit = &uarts[id];

    uint32_t iirId;
    do {
        // Interrupt Identification Register[IIR_ID]
        iirId = ReadReg32(unit->baseAddr, 0x08) & 0x1F;
        switch (iirId) {
        case 0x01: // No interrupt pending
            break;
            // The TX FIFO can accept more data.
        case 0x02: { // TX Holding Register Empty Interrupt
            EnqCtrType localEnqueuedBytes = unit->txEnqueuedBytes;
            EnqCtrType localDequeuedBytes = unit->txDequeuedBytes;

            // TX_OFFSET, holds number of bytes in TX FIFO.
            uint32_t txOffset = ReadReg32(unit->baseAddr, 0x6C);
            uint32_t spaceInTxFifo = TX_FIFO_DEPTH - txOffset;

            while (localDequeuedBytes != localEnqueuedBytes && spaceInTxFifo > 0) {
                EnqCtrType txIdx = localDequeuedBytes & TX_BUFFER_MASK;
                // TX Holding Register
                WriteReg32(unit->baseAddr, 0x00, unit->txBuffer[txIdx]);

                ++localDequeuedBytes;
                --spaceInTxFifo;
            }

            // If sent all enqueued data then disable TX interrupt.
            if (localEnqueuedBytes == localDequeuedBytes) {
                // Interrupt Enable Register
                ClearReg32(unit->baseAddr, 0x04, 0x02);
            }
            unit->txDequeuedBytes = localDequeuedBytes;
        } break;

        // Read from the FIFO if it has passed its trigger level, or if a timeout
        // has occurred, meaning there is unread data still in the FIFO.
        case 0x0C:   // RX Data Timeout Interrupt
        case 0x04: { // RX Data Received Interrupt
            EnqCtrType localEnqueuedBytes = unit->rxEnqueuedBytes;
            EnqCtrType localDequeuedBytes = unit->rxDequeuedBytes;

            EnqCtrType availSpace;
            if (localEnqueuedBytes >= localDequeuedBytes) {
                availSpace = RX_BUFFER_SIZE - (localEnqueuedBytes - localDequeuedBytes);
            }
            // If counter wrapped around, work out true remaining space.
            else {
                availSpace = (localDequeuedBytes & RX_BUFFER_MASK) - localEnqueuedBytes;
            }

            // LSR[0] = 1 -> Data Ready
            while (availSpace > 0 && (ReadReg32(unit->baseAddr, 0x14) & 0x01)) {
                EnqCtrType idx = localEnqueuedBytes & RX_BUFFER_MASK;
                // RX Buffer Register
                unit->rxBuffer[idx] = ReadReg32(unit->baseAddr, 0x00);

                ++localEnqueuedBytes;
                --availSpace;
            }

            unit->rxEnqueuedBytes = localEnqueuedBytes;

            if (unit->rxCallback) {
                unit->rxCallback();
            }
        } break;
        } // switch (iirId) {
    } while (iirId != 0x01);
}

bool Uart_EnqueueData(UartId id, const uint8_t *data, size_t length)
{
    UartInfo *unit = &uarts[id];

    EnqCtrType localEnqueuedBytes = unit->txEnqueuedBytes;
    EnqCtrType localDequeuedBytes = unit->txDequeuedBytes;

    EnqCtrType availSpace;
    if (localEnqueuedBytes >= localDequeuedBytes) {
        availSpace = TX_BUFFER_SIZE - (localEnqueuedBytes - localDequeuedBytes);
    }
    // If counter wrapped around, work out true remaining space.
    else {
        availSpace = (localDequeuedBytes & TX_BUFFER_MASK) - localEnqueuedBytes;
    }

    // If no available space then do not enable TX interrupt.
    if (availSpace == 0) {
        return false;
    }

    // Copy as much data as possible from the message to the buffer.
    // Any unqueued data will be lost.
    bool writeAll = (availSpace >= length);
    EnqCtrType bytesToWrite = writeAll ? length : availSpace;

    while (bytesToWrite--) {
        EnqCtrType idx = localEnqueuedBytes & TX_BUFFER_MASK;
        unit->txBuffer[idx] = *data++;
        ++localEnqueuedBytes;
    }

    // Block IRQs here because the the UART IRQ could already be enabled, and run
    // between updating txEnqueuedBytes and re-enabling the IRQ here. If that happened,
    // the IRQ could exhaust the software buffer and disable the TX interrupt, only
    // for it to be re-enabled here, in which case it would not get cleared because
    // there was no data to write to the TX FIFO.
    uint32_t prevPriBase = BlockIrqs();
    unit->txEnqueuedBytes = localEnqueuedBytes;
    // IER[ETBEI] = 1 -> Enable Transmitter Buffer Empty Interrupt
    SetReg32(unit->baseAddr, 0x04, 0x02);
    RestoreIrqs(prevPriBase);

    return writeAll;
}

size_t Uart_DequeueData(UartId id, uint8_t *buffer, size_t bufferSize)
{
    UartInfo *unit = &uarts[id];

    EnqCtrType localEnqueuedBytes = unit->rxEnqueuedBytes;
    EnqCtrType localDequeuedBytes = unit->rxDequeuedBytes;

    EnqCtrType availData;
    if (localEnqueuedBytes >= localDequeuedBytes) {
        availData = localEnqueuedBytes - localDequeuedBytes;
    }
    // Wraparound occurred so work out the true available data.
    else {
        availData = RX_BUFFER_SIZE - ((localDequeuedBytes & RX_BUFFER_MASK) - localEnqueuedBytes);
    }

    // This check is required to distinguish an empty buffer from a full buffer, because
    // in both cases the enqueue and dequeue indices point to the same index.
    if (availData == 0) {
        return 0;
    }

    EnqCtrType enqueueIndex = localEnqueuedBytes & RX_BUFFER_MASK;
    EnqCtrType dequeueIndex = localDequeuedBytes & RX_BUFFER_MASK;

    // If the available data does not wraparound use one memcpy...
    if (enqueueIndex > dequeueIndex) {
        __builtin_memcpy(buffer, &unit->rxBuffer[dequeueIndex], availData);
    }
    // ...otherwise copy data from end of buffer, then from start.
    else {
        size_t bytesFromEnd = RX_BUFFER_SIZE - dequeueIndex;
        __builtin_memcpy(buffer, &unit->rxBuffer[dequeueIndex], bytesFromEnd);
        __builtin_memcpy(buffer + bytesFromEnd, &unit->rxBuffer[0], enqueueIndex);
    }

    unit->rxDequeuedBytes += availData;
    return availData;
}

bool Uart_EnqueueString(UartId id, const char *msg)
{
    return Uart_EnqueueData(id, (const uint8_t *)msg, __builtin_strlen(msg));
}

static bool EnqueueIntegerAsStringWithBase(UartId id, int value, int base)
{
    // Maximum decimal length is minus sign followed by ten digits.
    char txt[1 + 10];
    char *p = txt;

    bool isNegative = value < 0;
    if (isNegative) {
        *p++ = '-';
    }

    static const char digits[] = "0123456789abcdef";
    do {
        *p++ = digits[__builtin_abs(value % base)];
        value /= base;
    } while (value);

    // Reverse the digits, not including any negative sign.
    char *low = isNegative ? &txt[1] : &txt[0];
    char *high = p - 1;
    while (low < high) {
        char tmp = *low;
        *low = *high;
        *high = tmp;
        ++low;
        --high;
    }

    return Uart_EnqueueData(id, (const uint8_t *)txt, p - txt);
}

bool Uart_EnqueueIntegerAsString(UartId id, int value)
{
    return EnqueueIntegerAsStringWithBase(id, value, 10);
}

bool Uart_EnqueueIntegerAsHexString(UartId id, uint32_t value)
{
    // EnqueueIntegerAsStringWithBase takes a signed integer, so print each half-word
    // separately, so the value is not interpreted as negative integer if bit 31 is set.
    if (value & 0xFFFF0000) {
        return EnqueueIntegerAsStringWithBase(id, (int)(value >> 16), 16) &&
               Uart_EnqueueIntegerAsHexStringWidth(id, value, 4);
    }

    return EnqueueIntegerAsStringWithBase(id, value & 0x0000FFFF, 16);
}

bool Uart_EnqueueIntegerAsHexStringWidth(UartId id, uint32_t value, size_t width)
{
    while (width) {
        uint32_t printNybble = (value >> ((width - 1) * 4)) & 0xF;
        if (!EnqueueIntegerAsStringWithBase(id, printNybble, 16)) {
            return false;
        }
        --width;
    }

    return true;
}
