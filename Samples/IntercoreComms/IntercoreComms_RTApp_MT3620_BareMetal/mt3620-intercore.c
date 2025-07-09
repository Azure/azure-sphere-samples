/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "logical-dpc.h"

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"
#include "mt3620-uart-poll.h"

// Register locations and values.
static const uint32_t MAILBOX_COMMAND_OUTBOUND_BUFFER = 0xba5e0001;
static const uint32_t MAILBOX_COMMAND_INBOUND_BUFFER = 0xba5e0002;
static const uint32_t MAILBOX_COMMAND_END_OF_SETUP = 0xba5e0003;

static const uintptr_t MBOX_HSP_CA7_NORMAL_BASE = 0x21050000;

static const size_t SW_RX_INT_STS_OFFSET = 0x1C;
static const uint32_t SW_RX_INT_HLCORE_SENT_TO_IOCORE = 0x2;

static const size_t SW_RX_INT_EN_OFFSET = 0x18;

static const size_t SW_TX_INT_PORT = 0x14;
static const uint32_t SW_MBOX_EVENT_IOCORE_SENT_TO_HLCORE = 0x1;
static const uint32_t SW_MBOX_EVENT_IOCORE_RECV_FROM_HLCORE = 0x2;

/// The mailbox interrupts run at this priority level.
static const uint32_t MBOX_PRIORITY = 2;

static CallbackNode recvCbNode;

static void ReceiveMessage(uint32_t *command, uint32_t *data);

// Helper function for MT3620_SetupIntercoreComm spins until it receives
// a message on the mailbox, and then retrieves the command and data.
static void ReceiveMessage(uint32_t *command, uint32_t *data)
{
    // FIFO_POP_CNT
    while (ReadReg32(MBOX_HSP_CA7_NORMAL_BASE, 0x58) == 0) {
        // empty.
    }

    // DATA_POP0
    *data = ReadReg32(MBOX_HSP_CA7_NORMAL_BASE, 0x54);
    // CMD_POP0
    *command = ReadReg32(MBOX_HSP_CA7_NORMAL_BASE, 0x50);
}

void MT3620_SetupIntercoreComm(uint32_t *inboundBase, uint32_t *outboundBase, Callback recvCallback)
{
    recvCbNode.enqueued = false;
    recvCbNode.next = NULL;
    recvCbNode.cb = recvCallback;

    // Wait for the mailbox to be set up.
    while (true) {
        uint32_t cmd, data;
        ReceiveMessage(&cmd, &data);
        if (cmd == MAILBOX_COMMAND_OUTBOUND_BUFFER) {
            *outboundBase = data;
        } else if (cmd == MAILBOX_COMMAND_INBOUND_BUFFER) {
            *inboundBase = data;
        } else if (cmd == MAILBOX_COMMAND_END_OF_SETUP) {
            break;
        }
    }

    // Set up interrupt to be notified when high-level core sends a message to real-time core.
    WriteReg32(MBOX_HSP_CA7_NORMAL_BASE, SW_RX_INT_EN_OFFSET, SW_RX_INT_HLCORE_SENT_TO_IOCORE);
    WriteReg32(MBOX_HSP_CA7_NORMAL_BASE, SW_RX_INT_STS_OFFSET, SW_RX_INT_HLCORE_SENT_TO_IOCORE);

    SetNvicPriority(11, MBOX_PRIORITY);
    EnableNvicInterrupt(11);
}

void MT3620_HandleMailboxIrq11(void)
{
    EnqueueDeferredProc(&recvCbNode);

    // Clear the interrupt.
    WriteReg32(MBOX_HSP_CA7_NORMAL_BASE, SW_RX_INT_STS_OFFSET,
               SW_MBOX_EVENT_IOCORE_RECV_FROM_HLCORE);
}

void MT3620_SignalHLCoreMessageReceived(void)
{
    // Ensure memory transfers have completed (not just been sent) before raising interrupt.
    // "no instruction that appears in program order after the DSB instruction can execute until the
    // DSB completes" ARMv7M Architecture Reference Manual, ARM DDI 0403E.d S A3.7.3
    __asm__ volatile("dsb");

    WriteReg32(MBOX_HSP_CA7_NORMAL_BASE, SW_TX_INT_PORT, SW_MBOX_EVENT_IOCORE_RECV_FROM_HLCORE);
}

void MT3620_SignalHLCoreMessageSent(void)
{
    // Ensure memory writes have completed (not just been sent) before raising interrupt.
    // "no instruction that appears in program order after the DSB instruction can execute until the
    // DSB completes" ARMv7M Architecture Reference Manual, ARM DDI 0403E.d S A3.7.3
    __asm__ volatile("dsb");
    WriteReg32(MBOX_HSP_CA7_NORMAL_BASE, SW_TX_INT_PORT, SW_MBOX_EVENT_IOCORE_SENT_TO_HLCORE);
}
