/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was
done by Microsoft. See the LICENSE.txt in this directory, and for more background, see the README.md
for this sample. */

#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

// By default Azure Sphere Visual Studio-created applications define _POSIX_C_SOURCE  (in the
// vcxproj file). In this case we also depend on functions that libc provides which are not in
// POSIX, so we add _BSD_SOURCE
#define _BSD_SOURCE
#include <endian.h>

#include "../epoll_timerfd_utilities.h"
#include "../file_view.h"
#include "../mem_buf.h"

#include "crc.h"
#include "slip.h"
#include "dfu_uart_protocol.h"
#include "dfu_defs.h"

// Enable this to print the encoded data which is sent to the board.
//#define DUMP_TX_ENCODED

// Value used by the nRF52 bootloader to respond to a firmware version request.
#define IMAGE_TYPE_UNKNOWN 255

// Support functions.
static void LaunchRead(void);
static void ReadData(EventData *eventData);
static void LaunchWrite(void);
static void LaunchWriteThenRead(void);
static void WriteData(EventData *eventData);

static int StartTimeoutTimer(void);
static void CancelTimeoutTimer(void);
static void TimeoutTimerExpiredEvent(EventData *eventData);

static bool ValidateHeader(NrfDfuOpCode op);
static bool ValidateAndRemoveHeader(NrfDfuOpCode op);

static void MoveToNextDfuState(void);

static void CleanUpStateMachine(void);

static StateTransition HandleStart(void);
static void InitTimerExpiredEvent(EventData *eventData);
static StateTransition HandleInitTimerExpired(void);
static StateTransition HandlePingReceivedResponse(void);
static StateTransition HandlePrnReceivedResponse(void);
static StateTransition HandleMtuReceivedResponse(void);
static StateTransition HandleGetFirmwareDetails(void);
static StateTransition HandleFirmwareVersionReceivedResponse(void);
static StateTransition HandleSelectNextImage(void);

static StateTransition HandleInitPacketStart(void);
static StateTransition HandleInitPacketDoneSelectCommand(void);

static StateTransition HandleFirmwareStart(void);
static StateTransition HandleFirmwareDoneSelectData(void);

static StateTransition LaunchSelect(uint8_t objectType, DfuProtocolStates continueState);
static StateTransition HandleSelectReceivedSelectResponse(void);

static StateTransition TransferDataInFileViewWindow(uint8_t objectType,
                                                    DfuProtocolStates continueState);
static StateTransition HandleFileTransferReceivedCreateResponse(void);
static StateTransition HandleFileTransferSendNextFragmentFromFileView(void);
static StateTransition HandleFileTransferSentWriteObjectRequest(void);
static StateTransition HandleFileTransferReceivedWindowChecksumResponse(void);
static StateTransition HandleFileTransferReceivedExecuteResponse(void);

static StateTransition HandlePostValidateImage(void);
static void PostValidateTimerExpiredEvent(EventData *eventData);

static int CreateDisarmedTimer(EventData *eventData);
static int LaunchOneShotTimer(int fd, const struct timespec *delay);
static int CancelTimer(int fd);

// When the state machine completes successfully or otherwise,
// it calls the termination handler which is provided to ProgramImages.
static DfuResultHandler resultHandler = NULL;
static DfuResultStatus statusToReturn = DfuResult_Success;

struct DeviceTransferState dts;

// event handler data structures. Only the event handler field needs to be populated.
static EventData uartWriteEventData = {.eventHandler = &WriteData};
static EventData uartReadEventData = {.eventHandler = &ReadData};

// The state machine issues a ping request followed by an
// MTU request.  The MTU response contains the MTU value.
// Until this value is available, the buffer must be large
// enough to read responses from the device.
static const uint16_t PREAMBLE_MTU_SIZE = 16;

static int nrfUartFd = -1;
static int gpioResetFd = -1;
static int gpioDfuFd = -1;
static int epollFd = -1;

// Multiple images, e.g. soft device and application, can be written
// to the device.  These variables track which image is being written.
static size_t nextImageIndex = 0;
static size_t numberOfImages = 0;
static DfuImageData *allImages = NULL;
static const DfuImageData *currentImage = NULL;

// Tracks image number requested from nRF52
static uint8_t nrfImageIndex = 0;

void ProgramImages(DfuImageData *imagesToWrite, size_t imageCount, DfuResultHandler exitHandler)
{
    assert(exitHandler != NULL);

    // Fail if no image was provided.
    if (!imagesToWrite || imageCount == 0) {
        Log_Debug("ERROR:Invalid array of images.\n");
        exitHandler(DfuResult_Fail);
        return;
    }

    resultHandler = exitHandler;
    allImages = imagesToWrite;
    numberOfImages = imageCount;
    nextImageIndex = 0;
    nrfImageIndex = 0;
    for (unsigned int i = 0; i < numberOfImages; ++i) {
        allImages[i].isInstalled = false;
	}
    dts.state = DfuState_Start;
    MoveToNextDfuState();
}

void InitUartProtocol(int openedUartFd, int openedResetFd, int openedDfuFd, int openedEpollFd)
{
    nrfUartFd = openedUartFd;
    gpioResetFd = openedResetFd;
    gpioDfuFd = openedDfuFd;
    epollFd = openedEpollFd;
    dts.state = DfuState_Start;
    dts.mtu = PREAMBLE_MTU_SIZE;
}

/// <summary>
/// Encodes the header and (optionally) the payload.
/// </summary>
/// <param name="op">Type of request to send.</param>
/// <param name="buf">Start of payload data.  Can be NULL.</param>
/// <param name="len">Length of payload data.  Not used if buf is NULL.</param>
static void EncodeHeaderAndOptionalPayload(NrfDfuOpCode op, const uint8_t *buf, size_t len)
{
    // Encode header.
    MemBufReset(dts.txBuf);
    uint8_t op8 = (uint8_t)op;
    SlipEncodeAppend(dts.txBuf, &op8, sizeof(op8));

    // Encode payload if required.
    if (buf) {
        SlipEncodeAppend(dts.txBuf, buf, len);
    }
    SlipEncodeAddEndMarker(dts.txBuf);

#ifdef DUMP_TX_ENCODED
    MemBufDump(dts.txBuf, "Slip TX.Wire");
#endif
}

// Encode a request without a payload.
static void EncodeHeaderOnly(NrfDfuOpCode op)
{
    EncodeHeaderAndOptionalPayload(op, NULL, 0);
}

// Encode a request with a payload.
static void EncodeHeaderAndPayload(NrfDfuOpCode op, const uint8_t *buf, size_t len)
{
    EncodeHeaderAndOptionalPayload(op, buf, len);
}

/// <summary>
/// Tests whether the received response contains a valid header,
/// and whether that header indicates success.
/// </summary>
/// <param name="op">The response should be for this operation.</param>
/// <returns>true if the expected header is present, valid, and successful;
/// false otherwise.</returns>
static bool ValidateHeader(NrfDfuOpCode op)
{
    // The received data must be at least three bytes long
    // to contain a valid header.
    const uint8_t *data;
    size_t extent;
    MemBufData(dts.decodedRxBuf, &data, &extent);

    if (extent < 3) {
        return false;
    }

    uint8_t r0 = MemBufRead8(dts.decodedRxBuf, /* idx */ 0);
    uint8_t r1 = MemBufRead8(dts.decodedRxBuf, /* idx */ 1);
    uint8_t r2 = MemBufRead8(dts.decodedRxBuf, /* idx */ 2);

    bool asExpected = (r0 == NrfDfuOp_Response && r1 == op && r2 == NrfDfuRes_Success);
    if (r2 != NrfDfuRes_Success) {
        Log_Debug("ERROR: Bootloader returned error code: 0x%02hhX.\n", r2);
    }
    return asExpected;
}

/// <summary>
/// Tests whether the received data contains an expected, successful
/// header.  If so, it removes the header and shifts the payload down
// to the start of the buffer.
/// </summary>
/// <param name="op">The response should be for this operation.</param>
/// <returns>true if the expected header is present, valid, and successful;
/// false otherwise.</returns>
static bool ValidateAndRemoveHeader(NrfDfuOpCode op)
{
    if (!ValidateHeader(op)) {
        return false;
    }

    // Header is always three bytes.
    MemBufShiftLeft(dts.decodedRxBuf, 3);
    return true;
}

/// <summary>
/// <para>Resets the state machine's read buffer and reads a packet from
/// the device.  The incoming packet will be SLIP-encoded, but is
/// stored in dts.decodedRxBuf in decoded form.</para>
///
/// <para>If the read completes successfully, the state machine will advance
/// to dts.state.  If an error occurs, the state machine will be advanced to
/// DfuState_Failed.</para>
/// </summary>
static void LaunchRead(void)
{
    dts.bytesRead = 0;
    dts.decodeState = NRF_SLIP_STATE_DECODING;
    MemBufReset(dts.decodedRxBuf);

    ReadData(NULL);
}

/// <summary>
/// <para>Called to launch a read or to continue a previously-started read.
/// If the entire packet is not available, this function will not block,
/// but will return to the epoll event handler.  It will be called again
/// when more data becomes available.</para>
///
/// <para>When an entire packet has been successfully read, this function will
/// advance the state machine to dts.state. If an error occurs, the state machine
/// will be transitioned to DfuState_Failed. This function uses the global UART file descriptor
/// as well as the global read event handler structure.</para>
/// </summary>
static void ReadData(EventData *eventData)
{
    if (dts.epollinEnabled) {
        CancelTimeoutTimer();
        UnregisterEventHandlerFromEpoll(epollFd, nrfUartFd);
        dts.epollinEnabled = false;
    }

    bool finished = false;
    while (!finished && dts.bytesRead < dts.mtu) {
        // Read a single byte from the UART and decode it.
        uint8_t b;
        ssize_t bytesReadOneSysCall = read(nrfUartFd, &b, 1);

        // Successfully read a single byte.
        if (bytesReadOneSysCall == 1) {
            ++dts.bytesRead;

            SlipDecodeAddByte(b, dts.decodedRxBuf, &dts.decodeState, &finished);

            // If the incoming data could not be decoded then abort the transfer.
            if (dts.decodeState == NRF_SLIP_STATE_CLEARING_INVALID_PACKET) {
                dts.state = DfuState_Failed;
                finished = true;
            }
        }

        // If receive buffer is empty then stay in current state and wait for EPOLLIN.
        else if ((bytesReadOneSysCall == 0) || (bytesReadOneSysCall < 0 && errno == EAGAIN)) {
            if (StartTimeoutTimer() == -1) {
                dts.state = DfuState_Failed;
                break;
            }

            // Return rather than transition to next state.
            RegisterEventHandlerToEpoll(epollFd, nrfUartFd, &uartReadEventData, EPOLLIN);
            dts.epollinEnabled = true;
            return;
        }

        // Another error occured so abort the transfer.
        else {
            dts.state = DfuState_Failed;
            break;
        }
    }

    // If received full mtu of bytes and Slip data has not yet
    // finished, then an error has occured so abort the transfer.
    if (!finished && dts.bytesRead == dts.mtu) {
        dts.state = DfuState_Failed;
    }

    // receive finished - move to next DFU state
    MoveToNextDfuState();
}

/// <summary>
/// <para>Writes data in dts.txBuf to the attached board.
/// The data must be in SLIP-encoded format.  If data cannot
/// be immediately written because an underlying buffer is
/// full this function will return to the epoll event loop,
/// which will call it again when there is space in the buffer.</para>
///
/// <para>If the write completes successfully, this function
/// will transition the state machine to dts.state.  If an error
/// occurs then it will transition the state machine to
/// DfuState_Failed.</para>
/// </summary>
static void LaunchWrite(void)
{
    dts.bytesSent = 0;
    dts.readAfterWrite = false;

    WriteData(NULL);
}

/// <summary>
/// <para>Writes data in dts.txBuf to the attached board.
/// The data must be in SLIP-encoded format.  If data cannot
/// be immediately written because an underlying buffer is
/// full this function will return to the epoll event loop,
/// which will call it again when there is space in the buffer.</para>
///
/// <para>If the write completes successfully, this function
/// will will immediately launch a read.  If the read completes
/// successfully, then the state machine will be transitioned to
/// dts.state.  If an error occurs then it will transition the state
/// machine to DfuState_Failed.</para>
/// </summary>
static void LaunchWriteThenRead(void)
{
    dts.bytesSent = 0;
    dts.readAfterWrite = true;

    WriteData(NULL);
}

/// <summary>
///	This function is called by LaunchWrite or LaunchWriteThenRead to
/// write data in dts.txBuf.  If it cannot write data because the
/// underlying buffer is full, then it will return to the epoll event
/// handler, which will call it when there is space in the buffer.
/// This function uses the global UART file descriptor as well as
/// the global write event handler structure.
/// </summary>
static void WriteData(EventData *eventData)
{
    if (dts.epolloutEnabled) {
        CancelTimeoutTimer();
        UnregisterEventHandlerFromEpoll(epollFd, nrfUartFd);
        dts.epolloutEnabled = false;
    }

    // Continue to fill the UART buffer while there is data remaining
    // and while the buffer is not full.
    while (dts.bytesSent < MemBufCurSize(dts.txBuf)) {
        const uint8_t *data;
        size_t availBytes;
        MemBufData(dts.txBuf, &data, &availBytes);

        size_t remainingBytes = availBytes - dts.bytesSent;
        ssize_t bytesSent = write(nrfUartFd, &data[dts.bytesSent], remainingBytes);

        // If actually sent data then stay in the while loop and try
        // to send more data.
        if (bytesSent > 0) {
            dts.bytesSent += (size_t)bytesSent;
        }

        // Buffer is full so wait for EPOLLOUT.
        // Return rather than advance state machine to stay in current state.
        else if (bytesSent < 0 && errno == EAGAIN) {
            if (StartTimeoutTimer() == -1) {
                dts.state = DfuState_Failed;
                break;
            }

            RegisterEventHandlerToEpoll(epollFd, nrfUartFd, &uartWriteEventData, EPOLLOUT);
            dts.epolloutEnabled = true;
            return;
        }

        // Else another error occured so move to invalid state to abort transfer.
        // A return code of zero is interpreted as an error.
        else {
            dts.state = DfuState_Failed;
            break;
        }
    }

    // Write completed successfully or otherwise.
    if (dts.state != DfuState_Failed && dts.readAfterWrite) {
        LaunchRead();
    } else {
        MoveToNextDfuState();
    }
}

// Start a 5 second timer to identify timeout conditions.
static int StartTimeoutTimer(void)
{
    static const struct timespec timeoutDuration = {.tv_sec = 5, .tv_nsec = 0};
    if (LaunchOneShotTimer(dts.timeoutTimerEventData.fd, &timeoutDuration) == -1) {
        return -1;
    }

    return 0;
}

// Called when a read or write has occurred.
static void CancelTimeoutTimer(void)
{
    CancelTimer(dts.timeoutTimerEventData.fd);
}

static void TimeoutTimerExpiredEvent(EventData *eventData)
{
    ConsumeTimerFdEvent(dts.timeoutTimerEventData.fd);

    // Don't get notified if pending read or write completes after
    // this timer has expired.
    if (dts.epollinEnabled || dts.epolloutEnabled) {
        UnregisterEventHandlerFromEpoll(epollFd, nrfUartFd);
        dts.epollinEnabled = false;
        dts.epolloutEnabled = false;
    }

    dts.state = DfuState_Failed;

    Log_Debug("ERROR: Could not communicate with board.  Operation timed out.\n");
    MoveToNextDfuState();
}

/// <summary>
/// Calls the state handler for dts.state.  This may launch a read,
/// write, or read-then-write; cause an immediate transition; indicate
/// a failure; or indicate a successful termination.
/// </summary>
static void MoveToNextDfuState(void)
{
    StateTransition sttr;

    // This is set to true when this function should return to
    // its caller because the reprogramming operation has completed
    // successfully or otherwise, or because waiting for async IO
    // to complete.
    bool done = false;

    do {
        switch (dts.state) {
            // Preamble.
        case DfuState_Start:
            sttr = HandleStart();
            break;

        case DfuState_InitTimerExpired:
            sttr = HandleInitTimerExpired();
            break;

        case DfuState_PingReceivedResponse:
            sttr = HandlePingReceivedResponse();
            break;

        case DfuState_ReceiptNotificationReceivedResponse:
            sttr = HandlePrnReceivedResponse();
            break;

        case DfuState_MtuReceivedResponse:
            sttr = HandleMtuReceivedResponse();
            break;

        case DfuState_GetFirmwareDetails:
            sttr = HandleGetFirmwareDetails();
            break;

        case DfuState_FirmwareVersionReceivedResponse:
            sttr = HandleFirmwareVersionReceivedResponse();
            break;

        case DfuState_SelectNextImage:
            sttr = HandleSelectNextImage();
            break;

            // Init packet (.DAT) transfer.
        case DfuState_InitPacketStart:
            sttr = HandleInitPacketStart();
            break;

        case DfuState_InitPacketDoneSelectCommand:
            sttr = HandleInitPacketDoneSelectCommand();
            break;

            // Firmware (.BIN) transfer.
        case DfuState_FirmwareStart:
            sttr = HandleFirmwareStart();
            break;

        case DfuState_FirmwareDoneSelectData:
            sttr = HandleFirmwareDoneSelectData();
            break;

            // File transfer states common to .BIN and.DAT.
        case DfuState_FileTransferReceivedCreateResponse:
            sttr = HandleFileTransferReceivedCreateResponse();
            break;

        case DfuState_FileTransferSendNextFragmentFromFileView:
            sttr = HandleFileTransferSendNextFragmentFromFileView();
            break;

        case DfuState_FileTransferSentWriteObjectRequest:
            sttr = HandleFileTransferSentWriteObjectRequest();
            break;

        case DfuState_FileTrnasferReceivedWindowChecksumResponse:
            sttr = HandleFileTransferReceivedWindowChecksumResponse();
            break;

        case DfuState_FileTransferReceivedExecuteResponse:
            sttr = HandleFileTransferReceivedExecuteResponse();
            break;

            // Select command used by both transfers.
        case DfuState_SelectReceivedSelectResponse:
            sttr = HandleSelectReceivedSelectResponse();
            break;

        case DfuState_PostValidateImage:
            sttr = HandlePostValidateImage();
            break;

            // Terminal states.
        case DfuState_Success:
            statusToReturn = DfuResult_Success;
            sttr = StateTransition_Done;
            break;

        case DfuState_Failed:
            statusToReturn = DfuResult_Fail;
            sttr = StateTransition_Done;
            break;

        default:
            Log_Debug("Unrecognized state %d\n", dts.state);
            sttr = StateTransition_Done;
            assert(false);
            break;
        }

        // Launch async operation, immediately transition to next state,
        // or leave the state machine.
        switch (sttr) {
        case StateTransition_LaunchRead:
            LaunchRead();
            done = true;
            break;

        case StateTransition_LaunchWrite:
            LaunchWrite();
            done = true;
            break;

        case StateTransition_LaunchWriteThenRead:
            LaunchWriteThenRead();
            done = true;
            break;

        case StateTransition_Failed:
            dts.state = DfuState_Failed;
            break;

        case StateTransition_MoveImmediately:
            break;

        case StateTransition_WaitAsync:
            done = true;
            break;

        case StateTransition_Done:
            CleanUpStateMachine();
            // Exit DFU mode and restart the available firmware
            GPIO_SetValue(gpioDfuFd, GPIO_Value_High);
            GPIO_SetValue(gpioResetFd, GPIO_Value_Low);
            GPIO_SetValue(gpioResetFd, GPIO_Value_High);
            resultHandler(statusToReturn);
            done = true;
            return;

        default:
            Log_Debug("Unrecognized transition %d\n", sttr);
            assert(false);
            break;
        }
    } while (!done);
}

/// <summary>
/// Clean up any resources which were successfully allocated
/// by the state machine.
/// </summary>
static void CleanUpStateMachine(void)
{
    if (dts.initTimerEventData.fd != -1) {
        UnregisterEventHandlerFromEpoll(epollFd, dts.initTimerEventData.fd);
        CloseFdAndPrintError(dts.initTimerEventData.fd, "initTimer");
        dts.initTimerEventData.fd = -1;
    }

    if (dts.postValidateTimerEventData.fd != -1) {
        UnregisterEventHandlerFromEpoll(epollFd, dts.postValidateTimerEventData.fd);
        CloseFdAndPrintError(dts.postValidateTimerEventData.fd, "postValidateTimer");
        dts.postValidateTimerEventData.fd = -1;
    }

    if (dts.timeoutTimerEventData.fd != -1) {
        UnregisterEventHandlerFromEpoll(epollFd, dts.timeoutTimerEventData.fd);
        CloseFdAndPrintError(dts.timeoutTimerEventData.fd, "timeoutTimer");
        dts.timeoutTimerEventData.fd = -1;
    }

    CloseFileView(dts.fv);
    dts.fv = NULL;

    FreeMemBuf(dts.txBuf);
    dts.txBuf = NULL;

    FreeMemBuf(dts.decodedRxBuf);
    dts.decodedRxBuf = NULL;
}

// Called on DfuState_Start.
//
/// Allocates resources required to send images and puts attached
/// nRF52 board into DFU mode.
static StateTransition HandleStart(void)
{
    // Mark resources as unused so they can be safely cleaned up if an
    // error occurs before they are all initialized.
    dts.txBuf = NULL;
    dts.decodedRxBuf = NULL;
    dts.fv = NULL;

    dts.initTimerEventData.eventHandler = &InitTimerExpiredEvent;
    dts.initTimerEventData.fd = -1;

    dts.postValidateTimerEventData.eventHandler = &PostValidateTimerExpiredEvent;
    dts.postValidateTimerEventData.fd = -1;

    dts.timeoutTimerEventData.eventHandler = &TimeoutTimerExpiredEvent;
    dts.timeoutTimerEventData.fd = -1;

    dts.epollinEnabled = false;
    dts.epolloutEnabled = false;

    // These buffer sizes are large enough to send the ping
    // and request the MTU size.  They will be adjusted once the
    // actual MTU size has been retrieved from the device.
    dts.txBuf = AllocMemBuf(PREAMBLE_MTU_SIZE);

    if (!dts.txBuf) {
        return StateTransition_Failed;
    }

    dts.decodedRxBuf = AllocMemBuf(PREAMBLE_MTU_SIZE);
    if (!dts.decodedRxBuf) {
        return StateTransition_Failed;
    }

    // Create all of the required timers in disarmed state.
    dts.initTimerEventData.fd = CreateDisarmedTimer(&dts.initTimerEventData);
    if (dts.initTimerEventData.fd == -1) {
        return StateTransition_Failed;
    }

    dts.postValidateTimerEventData.fd = CreateDisarmedTimer(&dts.postValidateTimerEventData);
    if (dts.postValidateTimerEventData.fd == -1) {
        return StateTransition_Failed;
    }

    dts.timeoutTimerEventData.fd = CreateDisarmedTimer(&dts.timeoutTimerEventData);
    if (dts.timeoutTimerEventData.fd == -1) {
        return StateTransition_Failed;
    }

    dts.pingId = 1;

    // Put the nRF52 into DFU mode.
    GPIO_SetValue(gpioResetFd, GPIO_Value_Low);
    GPIO_SetValue(gpioDfuFd, GPIO_Value_Low);
    GPIO_SetValue(gpioResetFd, GPIO_Value_High);
 
    // Wait one second for nRF52 to go into DFU mode.
    static const struct timespec initTimerDuration = {.tv_sec = 1, .tv_nsec = 0};
    if (LaunchOneShotTimer(dts.initTimerEventData.fd, &initTimerDuration) == -1) {
        return StateTransition_Failed;
    }

    // Do not set next state - that happens in InitTimerExpiredEvent.
    return StateTransition_WaitAsync;
}

// Called by epoll event handler when timer expires.
// Consumes one-shot timer event but does not close the timer.
static void InitTimerExpiredEvent(EventData *eventData)
{
    bool consumed = (ConsumeTimerFdEvent(dts.initTimerEventData.fd) == 0);
    dts.state = consumed ? DfuState_InitTimerExpired : DfuState_Failed;

    MoveToNextDfuState();
}

// Called on DfuState_InitTimerExpired.
static StateTransition HandleInitTimerExpired(void)
{
    // At this point the nRF52 should not be sending any data so
    // clear any previously-sent data from the OS receive buffer.

    bool cleared = false;
    do {
        uint8_t b;
        int r = read(nrfUartFd, &b, 1);

        // If a read error occurred then abort.
        if (r < 0) {
            return StateTransition_Failed;
        }

        // If no data was read then have exhausted the OS receive
        // buffer so stop reading from the UART.
        else if (r == 0) {
            cleared = true;
        }

        // Else a byte was read from the buffer, so iterate again.
    } while (!cleared);

    // Send the ping command.
    ++dts.pingId;
    EncodeHeaderAndPayload(NrfDfuOp_Ping, &dts.pingId, 1);

    dts.state = DfuState_PingReceivedResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on DfuState_PingReceivedResponse.
static StateTransition HandlePingReceivedResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_Ping)) {
        return StateTransition_Failed;
    }

    // Payload should contain a one-byte ping id.
    if (MemBufCurSize(dts.decodedRxBuf) != 1) {
        return StateTransition_Failed;
    }

    // Ensure the ping id in the payload is equal to the ping id that was sent.
    uint8_t receivedPingId = MemBufRead8(dts.decodedRxBuf, /* idx */ 0);
    if (receivedPingId != dts.pingId) {
        return StateTransition_Failed;
    }

    // Send the packet receipt notification (PRN).
    dts.prn = 0;
    uint16_t sendPrn = htole16(dts.prn);
    EncodeHeaderAndPayload(NrfDfuOp_ReceiptNotificationSet, (const uint8_t *)&sendPrn, 2);

    dts.state = DfuState_ReceiptNotificationReceivedResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on DfuState_ReceiptNotificationReceivedResponse.
static StateTransition HandlePrnReceivedResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_ReceiptNotificationSet)) {
        return StateTransition_Failed;
    }

    // There should not be any payload with this response.
    if (MemBufCurSize(dts.decodedRxBuf) != 0) {
        return StateTransition_Failed;
    }

    // Request MTU from nRF52 board.
    EncodeHeaderOnly(NrfDfuOp_MtuGet);
    dts.state = DfuState_MtuReceivedResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on DfuState_MtuReceivedResponse.
static StateTransition HandleMtuReceivedResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_MtuGet)) {
        return StateTransition_Failed;
    }

    dts.mtu = MemBufReadLe16(dts.decodedRxBuf, 0);

    // Resize the buffers according to the available MTU size.
    // The TX buffer contains SLIP encoded payloads.  It should
    // be the same size as the MTU.  The source data is divided
    // up before it is encoded to ensure that it does not exceed
    // the MTU after it has been encoded.

    if (!MemBufResize(dts.txBuf, dts.mtu)) {
        return StateTransition_Failed;
    }

    // The RX buffer contains decoded payloads, and so will be
    // no longer than the MTU.
    if (!MemBufResize(dts.decodedRxBuf, dts.mtu)) {
        return StateTransition_Failed;
    }

    // if the nextImageIndex is greater than 0
    // then the image isInstalled and installedVersion 
	// fields have been set for all images which 
    // have to be updated
    if (nextImageIndex != 0) {
        dts.state = DfuState_SelectNextImage;
    }
    // otherwise, the version of each image has to be
    // checked and the isInstalled and installedVersion fields 
    // have to be set accordingly
    else {
        Log_Debug("Requesting details of firmware present on nRF52:\n");
        dts.state = DfuState_GetFirmwareDetails;
    }
    return StateTransition_MoveImmediately;
}

// called on DfuState_GetFirmwareDetails
static StateTransition HandleGetFirmwareDetails(void)
{
    EncodeHeaderAndOptionalPayload(NrfDfuOp_FirmwareVersion, &nrfImageIndex, 1);
    nrfImageIndex++;
    dts.state = DfuState_FirmwareVersionReceivedResponse;
    return StateTransition_LaunchWriteThenRead;
}

// called on DfuState_FirmwareVersionReceivedResponse
static StateTransition HandleFirmwareVersionReceivedResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_FirmwareVersion)) {
        return StateTransition_Failed;
    }

    size_t currentOffset = 0;
    uint8_t type = MemBufRead8(dts.decodedRxBuf, currentOffset);
    currentOffset += 1;
    uint32_t version = MemBufReadLe32(dts.decodedRxBuf, currentOffset);
    currentOffset += sizeof(version);
    uint32_t addr = MemBufReadLe32(dts.decodedRxBuf, currentOffset);
    currentOffset += sizeof(addr);
    uint32_t len = MemBufReadLe32(dts.decodedRxBuf, currentOffset);

    // Unknown image type means no more images are present on the nRF52
    if (type == IMAGE_TYPE_UNKNOWN) {
        dts.state = DfuState_SelectNextImage;
        return StateTransition_MoveImmediately;
    }

    Log_Debug("Image %zu has type %" PRIu8 " version %" PRIu32 " address %" PRIu32 " size %" PRIu32
              ".\n",
              nrfImageIndex - 1, type, version, addr, len);

    for (unsigned int i = 0; i < numberOfImages; ++i) {
        if ((uint8_t)type == (uint8_t)allImages[i].firmwareType) {
            allImages[i].isInstalled = true;
            allImages[i].installedVersion = version;
            if (allImages[i].installedVersion != allImages[i].version) {
                Log_Debug("Image %s (%zu/%zu) with version %zu needs update to version %zu.\n",
                          allImages[i].datPathname, i + 1, numberOfImages, version,
                          allImages[i].version);
			}
		}
    }

    dts.state = DfuState_GetFirmwareDetails;
    return StateTransition_MoveImmediately;
}

// Called on DfuState_SelectNextImage.
static StateTransition HandleSelectNextImage(void)
{
    while (nextImageIndex < numberOfImages) {
        currentImage = &(allImages[nextImageIndex]);
        nextImageIndex++;
        // if there is an image to add, it will be added
        if (!currentImage->isInstalled) {
            Log_Debug("Adding image %s (%zu/%zu) with version %zu.\n", currentImage->datPathname,
                      nextImageIndex, numberOfImages, currentImage->version);
            dts.state = DfuState_InitPacketStart;
            break;
        }
        // if there is an image to update, it will be updated
        if (currentImage->installedVersion != currentImage->version) {
            Log_Debug("Updating image %s (%zu/%zu) from version %zu to version %zu.\n", currentImage->datPathname, nextImageIndex, numberOfImages,
                      currentImage->installedVersion, currentImage->version);
            dts.state = DfuState_InitPacketStart;
            break;
        }
        Log_Debug("Image %s (%zu/%zu) with version %zu doesn't need update.\n",
                  currentImage->datPathname, nextImageIndex, numberOfImages, currentImage->version);
    }

    // if no image needs update (including the last image), then the DFU update operation is aborted
    if (nextImageIndex >= numberOfImages && dts.state != DfuState_InitPacketStart) {
        Log_Debug("All images are up to date.\n");
        EncodeHeaderAndOptionalPayload(NrfDfuOp_Abort, NULL, 0);
        dts.state = DfuState_Success;
        return StateTransition_LaunchWrite;
    }

    return StateTransition_MoveImmediately;
}

// Called on INIT_PACKET_START.
static StateTransition HandleInitPacketStart(void)
{
    return LaunchSelect(0x01, DfuState_InitPacketDoneSelectCommand);
}

// Called on DfuState_InitPacketDoneSelectCommand.
static StateTransition HandleInitPacketDoneSelectCommand(void)
{
    // Open the init packet file and send send it to the nRF52.
    dts.fv = OpenFileView(currentImage->datPathname, dts.maxTxSize);
    if (!dts.fv) {
        Log_Debug("ERROR: Opening file %s failed with error code: %s (%d).\n",
                  currentImage->datPathname, strerror(errno), errno);
        return StateTransition_Failed;
    }

    // The init packet file must fit within a single transfer.
    off_t fileSize;
    FileViewFileOffsetSize(dts.fv, NULL, &fileSize);
    if (fileSize > (off_t)dts.maxTxSize) {
        return StateTransition_Failed;
    }

    if (!FileViewMoveWindow(dts.fv, 0)) {
        return StateTransition_Failed;
    }

    return TransferDataInFileViewWindow(0x1, DfuState_FirmwareStart);
}

// ---- Firmware (.DAT) programming states.

// Called on DfuState_FirmwareStart.
static StateTransition HandleFirmwareStart(void)
{
    return LaunchSelect(0x02, DfuState_FirmwareDoneSelectData);
}

// Called on DATA_DONE_SELECT_COMMAND.
static StateTransition HandleFirmwareDoneSelectData(void)
{
    // The init packet must fit within a single transfer so
    // open the init packet file and move to the start.
    dts.fv = OpenFileView(currentImage->binPathname, dts.maxTxSize);
    if (!dts.fv) {
        Log_Debug("ERROR: Opening file %s failed with error code: %s (%d).\n",
                  currentImage->binPathname, strerror(errno), errno);
        return StateTransition_Failed;
    }

    if (!FileViewMoveWindow(dts.fv, 0)) {
        return StateTransition_Failed;
    }

    return TransferDataInFileViewWindow(0x2, DfuState_PostValidateImage);
}

// ---- Functionality shared by init packet and data packet.

// Called to send a "select command" or "select data" request when the
// init packet or data packet are sent respectively.
static StateTransition LaunchSelect(uint8_t objectType, DfuProtocolStates continueState)
{
    EncodeHeaderAndPayload(NrfDfuOp_ObjectSelect, &objectType, sizeof(objectType));
    dts.selectContinueState = continueState;
    dts.state = DfuState_SelectReceivedSelectResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on COMMON_RECEIVED_SELECT_RESPONSE.
//
// On exit from this state, dts.maxTxSize and dts.runningCrc32
// have been updated with the values in the select response.
static StateTransition HandleSelectReceivedSelectResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_ObjectSelect)) {
        return StateTransition_Failed;
    }

    if (MemBufCurSize(dts.decodedRxBuf) != 12) {
        return StateTransition_Failed;
    }

    dts.maxTxSize = MemBufReadLe32(dts.decodedRxBuf, 0);

    // It only makes sense for offset == 0 at this point because
    // no file data has been transferred.  If the returned value
    // is not zero then abort.  This can happen if the device has
    // not fully reset since the last file was transferred.
    uint32_t offset = MemBufReadLe32(dts.decodedRxBuf, 4);
    if (offset != 0) {
        return StateTransition_Failed;
    }

    dts.runningCrc32 = MemBufReadLe32(dts.decodedRxBuf, 8);

    dts.state = dts.selectContinueState;
    return StateTransition_MoveImmediately;
}

// Called on DfuState_FileTransferReceivedCreateResponse.
static StateTransition TransferDataInFileViewWindow(uint8_t objectType,
                                                    DfuProtocolStates continueState)
{
    // Create an object.
    // For the init packet, this will be a command object; for the
    // firmware it will be a data object.

    off_t extent;
    FileViewWindow(dts.fv, /* data */ NULL, &extent);

    uint8_t buf[5];
    buf[0] = objectType;
    uint32_t lenLe = htole32((uint32_t)extent);
    memcpy(&buf[1], &lenLe, sizeof(lenLe));
    EncodeHeaderAndPayload(NrfDfuOp_ObjectCreate, buf, sizeof(buf));
    dts.fileTransferContinueState = continueState;
    dts.state = DfuState_FileTransferReceivedCreateResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on DfuState_FileTransferReceivedCreateResponse.
static StateTransition HandleFileTransferReceivedCreateResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_ObjectCreate)) {
        return StateTransition_Failed;
    }

    // The SLIP encoding can, in the worst case, double the payload
    // size and then add a terminator, so ensure there is enough space
    // in the MTU-sized buffer.
    dts.stepSize = (dts.mtu - 1) / 2 - 1;
    dts.offsetIntoFileView = 0;

    dts.state = DfuState_FileTransferSendNextFragmentFromFileView;
    return StateTransition_MoveImmediately;
}

// Called on DfuState_FileTransferSendNextFragmentFromFileView.
static StateTransition HandleFileTransferSendNextFragmentFromFileView(void)
{
    const uint8_t *data;
    off_t extent;
    FileViewWindow(dts.fv, &data, &extent);

    off_t bytesToSend = extent - dts.offsetIntoFileView;
    if (bytesToSend > dts.stepSize) {
        bytesToSend = dts.stepSize;
    }

    dts.fvFragmentLen = bytesToSend;

    const uint8_t *dataToSend = &data[dts.offsetIntoFileView];
    EncodeHeaderAndPayload(NrfDfuOp_ObjectWrite, dataToSend, (size_t)bytesToSend);

    dts.runningCrc32 = CalcCrc32WithSeed(dataToSend, (size_t)bytesToSend, dts.runningCrc32);

    dts.state = DfuState_FileTransferSentWriteObjectRequest;
    return StateTransition_LaunchWrite;
}

// Called on HandleFileTransferSentWriteObjectRequest.
static StateTransition HandleFileTransferSentWriteObjectRequest(void)
{
    // No response to check.

    dts.offsetIntoFileView += dts.fvFragmentLen;

    // If data remaining in file view, then send next fragment.
    off_t extent;
    FileViewWindow(dts.fv, /* data */ NULL, &extent);
    if (dts.offsetIntoFileView < extent) {
        dts.state = DfuState_FileTransferSendNextFragmentFromFileView;
        return StateTransition_MoveImmediately;
    }

    // Have sent all data in file view, so ask for a checksum.
    EncodeHeaderOnly(NrfDfuOp_CrcGet);
    dts.state = DfuState_FileTrnasferReceivedWindowChecksumResponse;
    return StateTransition_LaunchWriteThenRead;
}

// DfuState_FileTrnasferReceivedWindowChecksumResponse
static StateTransition HandleFileTransferReceivedWindowChecksumResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_CrcGet)) {
        return StateTransition_Failed;
    }

    // Check whether the reported offset and CRC match the expected values.
    uint32_t reportedOffset = MemBufReadLe32(dts.decodedRxBuf, 0);
    uint32_t reportedCrc32 = MemBufReadLe32(dts.decodedRxBuf, 4);

    // Have just sent another window's worth of data from the
    // file, so ensure the offset matches the expected file position.

    off_t fileOffset;
    FileViewFileOffsetSize(dts.fv, &fileOffset, /* size */ NULL);
    off_t windowExtent;
    FileViewWindow(dts.fv, /* data */ NULL, &windowExtent);

    if (reportedOffset != fileOffset + windowExtent) {
        return StateTransition_Failed;
    }

    if (reportedCrc32 != dts.runningCrc32) {
        return StateTransition_Failed;
    }

    // Send the execute opcode.
    EncodeHeaderOnly(NrfDfuOp_ObjectExecute);
    dts.state = DfuState_FileTransferReceivedExecuteResponse;
    return StateTransition_LaunchWriteThenRead;
}

// Called on DfuState_FileTransferReceivedExecuteResponse.
static StateTransition HandleFileTransferReceivedExecuteResponse(void)
{
    if (!ValidateAndRemoveHeader(NrfDfuOp_ObjectExecute)) {
        return StateTransition_Failed;
    }

    // If there is more data after the file view then move the
    // window and send the next block of data.
    off_t fileOffset;
    off_t fileSize;
    FileViewFileOffsetSize(dts.fv, &fileOffset, &fileSize);
    off_t windowExtent;
    FileViewWindow(dts.fv, /* data */ NULL, &windowExtent);

    if (fileOffset + windowExtent < fileSize) {
        FileViewMoveWindow(dts.fv, fileOffset + windowExtent);
        dts.state = DfuState_FileTransferSendNextFragmentFromFileView;
        dts.offsetIntoFileView = 0;
        return TransferDataInFileViewWindow(0x2, DfuState_PostValidateImage);
    }

    CloseFileView(dts.fv);
    dts.fv = NULL;

    dts.state = dts.fileTransferContinueState;
    return StateTransition_MoveImmediately;
}

// Called on DfuState_PostValidateImage.
//
// Waits for DFU to postvalidate the updated image.
static StateTransition HandlePostValidateImage(void)
{
    // Finished sending an image update, so wait for postvalidation on DFU side.
    // the waiting time differs based on the firmware type
    time_t waitTime = 1;
    if (currentImage->firmwareType == DfuFirmware_Softdevice) {
        waitTime = 5;
    }

    const struct timespec postValidateTimerDuration = {.tv_sec = waitTime, .tv_nsec = 0};
    if (LaunchOneShotTimer(dts.postValidateTimerEventData.fd, &postValidateTimerDuration) == -1) {
        return StateTransition_Failed;
    }

    Log_Debug("Waiting for image %s postvalidation\n", currentImage->datPathname);
    // Do not set next state - that happens in postValidateTimerExpiredEvent.
    return StateTransition_WaitAsync;
}

static void PostValidateTimerExpiredEvent(EventData *eventData)
{
    bool consumed = (ConsumeTimerFdEvent(dts.postValidateTimerEventData.fd) == 0);
    dts.state = consumed ? DfuState_Success : DfuState_Failed;

    // check if there are images which have to be added or updated
    for (size_t i = nextImageIndex; i < numberOfImages && dts.state != DfuState_Failed; ++i) {
        if (!allImages[i].isInstalled || (allImages[i].installedVersion != allImages[i].version)) {
            dts.state = DfuState_Start;
            CleanUpStateMachine();
            break;
        }
    }

    MoveToNextDfuState();
}

static int CreateDisarmedTimer(EventData *eventData)
{
    // Setting both fields to zero disarms the timer.
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
    int fd = CreateTimerFdAndAddToEpoll(epollFd, &ts, eventData, EPOLLIN);
    return fd;
}

static int LaunchOneShotTimer(int fd, const struct timespec *delay)
{
    return SetTimerFdToSingleExpiry(fd, delay);
}

static int CancelTimer(int fd)
{
    // Setting both fields to zero disarms the timer.
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
    return LaunchOneShotTimer(fd, &ts);
}
