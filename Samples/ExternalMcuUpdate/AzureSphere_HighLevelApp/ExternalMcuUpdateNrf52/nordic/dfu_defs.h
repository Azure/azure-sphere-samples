/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was done by Microsoft. See the
LICENSE.txt in this directory, and for more background, see the README.md for this sample. */

#pragma once

#include <unistd.h>
#include <stdint.h>

#include "../file_view.h"
#include "../mem_buf.h"
#include "../epoll_timerfd_utilities.h"

#include "slip.h"

/// <summary>
/// These opcodes are included in the headers for requests sent to and responses
/// received from the attached board. The set of opcodes is the same as the one
/// used in the nRF52 bootloader solution.
/// </summary>
typedef enum {
    /// <summary>Create an init packet or firmware object.</summary>
    NrfDfuOp_ObjectCreate = 0x01,

    /// <summary>Set the packet receipt notification value.</summary>
    NrfDfuOp_ReceiptNotificationSet = 0x02,

    /// <summary>
    /// Return offset and running CRC-32 for data which has been written
    /// with NrfDfuOp_ObjectWrite.
    /// <seealso cref="NrfDfuOp_ObjectWrite" />
    /// </summary>
    NrfDfuOp_CrcGet = 0x03,

    /// <summary>
    /// The attached board supports a maximum transfer size, which is the
    /// maximum number of bytes that can be written before the execute request
    /// is sent. (This value may be greater than the MTU, and the data may be
    /// written in multiple write operations.) Once this amount of data has
    /// been transferred to the board, the MT3620 requests a checksum and, if
    /// that is successful, sends an execute command.
    /// </summary>
    NrfDfuOp_ObjectExecute = 0x04,

    /// <summary>Used to select init packet or firmware for subsequent download.</summary>
    NrfDfuOp_ObjectSelect = 0x06,

    /// <summary>Get number of bytes that can be sent at once.</summary>
    NrfDfuOp_MtuGet = 0x07,

    /// <summary>Tell the device to receive data for the init packet or firmware.</summary>
    NrfDfuOp_ObjectWrite = 0x08,

    /// <summary>Request simple response from device to check whether it is present
    /// and communicating.</summary>
    NrfDfuOp_Ping = 0x09,

    /// <summary>Get the firmware version for an image.</summary>
    NrfDfuOp_FirmwareVersion = 0x0B,

    /// <summary>Abort the DFU procedure.</summary>
    NrfDfuOp_Abort = 0x0C,

    /// <summary>This must be the first byte of a response from the device.</summary>
    NrfDfuOp_Response = 0x60,

    /// <summary>Unused.</summary>
    NrfDfuOp_Invalid = 0xFF,
} NrfDfuOpCode;

/// <summary>
/// The third byte in the header response contains a status code which uses
/// these values. Of these only NrfDfuRes_Success is explicitly tested for;
/// any other code is considered to be an error. The set of error codes, which
/// is the same as the one used in the nRF52 bootloader solution, is presented
/// here for reference if an unexpected response occurs.
/// </summary>
typedef enum {
    /// <summary>Invalid NrfDfuOpCode.</summary>
    NrfDfuRes_Invalid = 0x00,

    /// <summary>Operation successful. The third byte of each response must have this value,
    /// else the request has failed.</summary>
    NrfDfuRes_Success = 0x01,

    /// <summary><see cref="NrfDfuOpCode"/> not supported.</summary>
    NrfDfuRes_OpCodeNotSupported = 0x02,

    /// <summary>Missing or invalid parameter value.</summary>
    NrfDfuRes_InvalidParameter = 0x03,

    /// <summary>Not enough memory for the data object.</summary>
    NrfDfuRes_InsufficientResources = 0x04,

    /// <summary>Data object does not match the firmware and hardware requirements,
    /// the signature is wrong, or parsing the command failed.</summary>
    NrfDfuRes_InvalidObject = 0x05,

    /// <summary>Not a valid object type for a Create request.</summary>
    NrfDfuRes_UnsupportedType = 0x07,

    /// <summary>The state of the DFU process does not allow this operation.</summary>
    NrfDfuRes_OperationNotPermitted = 0x08,

    /// <summary>Operation failed.</summary>
    NrfDfuRes_OperationFailed = 0x0A,

    /// <summary>Extended error. The next byte of the response contains the error
    /// code of the extended error.</summary>
    NrfDfuRes_ExtendedError = 0x0B
} NrfDfuResCode;

/// <summary>
/// To be fully asynchronous, the attached board is programmed via a state machine.
/// The machine does not block on a read, write, or timer, but exits, and is resumed
/// when the event happens, successfully or otherwise.
/// </summary>
typedef enum {
    /// <summary>Initial state. This starts the process that writes all of the
    /// required images to the board.</summary>
    DfuState_Start,

    /// <summary>Terminal state is entered when all images have been written to
    /// the board.</summary>
    DfuState_Success,

    /// <summary>Terminal state is entered when a failure has occured.</summary>
    DfuState_Failed,

    /// <summary>Entered after a file has been written to the attached board.
    /// Launches a timer which gives the board time to consume the file.</summary>
    DfuState_PostValidateImage,

    /// <summary>A short timer is used to give the attached board some time to
    /// go into DFU mode before images can be written. This state is entered
    /// when that timer expires.</summary>
    DfuState_InitTimerExpired,

    /// <summary>Have received a ping response from the attached board.</summary>
    DfuState_PingReceivedResponse,

    /// <summary>Have received a PRN response from the attached board.</summary>
    DfuState_ReceiptNotificationReceivedResponse,

    /// <summary>Have received an MTU response from the attached board.</summary>
    DfuState_MtuReceivedResponse,

    /// <summary>Asks for firmware type and version.</summary>
    DfuState_GetFirmwareDetails,

    /// <summary>Have received a firmware version from the attached board.</summary>
    DfuState_FirmwareVersionReceivedResponse,

    /// <summary>Select the next image to update, or abort if no images need updating.</summary>
    DfuState_SelectNextImage,

    /// <summary>Start writing the init packet file to the attached board.</summary>
    DfuState_InitPacketStart,

    /// <summary>Have asked board to begin receiving init packet data.</summary>
    DfuState_InitPacketDoneSelectCommand,

    /// <summary>Start writing the firmware file to the attached board.</summary>
    DfuState_FirmwareStart,

    /// <summary>Have asked board to begin receiving firmware data.</summary>
    DfuState_FirmwareDoneSelectData,

    /// <summary>Have received response to NrfDfuOp_ObjectSelect request.</summary>
    DfuState_SelectReceivedSelectResponse,

    /// <summary>Have received response to NrfDfuOp_ObjectCreate request.</summary>
    DfuState_FileTransferReceivedCreateResponse,

    /// <summary>Have written up to one MTU of data, so write next block from the file.</summary>
    DfuState_FileTransferSendNextFragmentFromFileView,

    /// <summary>Have received response to NrfDfuOp_ObjectWrite request.</summary>
    DfuState_FileTransferSentWriteObjectRequest,

    /// <summary>Have received response to NrfDfuOp_CrcGet request.</summary>
    DfuState_FileTrnasferReceivedWindowChecksumResponse,

    /// <summary>Have received response to NrfDfuOp_ObjectExecute request.</summary>
    DfuState_FileTransferReceivedExecuteResponse,
} DfuProtocolStates;

/// <summary>
/// The state handling functions return one of these values to
/// indicate how the state machine should transition to the next state.
/// The function must write the next state to dts.state before
/// returning one of these values. An exception is if it returns StateTransition_Failed,
/// then the state machine automatically goes to DfuState_Failed.
/// </summary>
typedef enum {
    /// <summary>
    /// <para>Launch an asynchronous read from the attached board.</para>
    /// <para>When the read completes successfully or otherwise, the
    /// state machine will transition to the next state.</para>
    /// <para>The read may be synchronous if the data is already available
    /// in the operating system receive buffer.</para>
    /// </summary>
    StateTransition_LaunchRead,

    /// <summary>
    /// <para>Launch an asynchronous read from the attached board.</para>
    /// <para>When the write completes successfully or otherwise, the
    /// state machine will transition to the next state.</para>
    /// <para>The write may be synchronous if there is enough space available
    /// in the operating system transmit buffer.</para>
    /// </summary>
    StateTransition_LaunchWrite,

    /// <summary>
    /// <para>This handles the common case where a request (write) is immediately
    /// followed by a response (read). The read is automatically launched
    /// when the write has successfully completed, and the state machine is
    /// advanced when the read completes.</para>
    /// <para>This avoids having to implement a state which only launches a read.</para>
    /// </summary>
    StateTransition_LaunchWriteThenRead,

    /// <summary>Move immediately to the state in dts.state.</summary>
    StateTransition_MoveImmediately,

    /// <summary>
    /// Wait for an external event that is neither a read nor a write.
    /// This is used to wait for a timer to expire.
    /// </summary>
    StateTransition_WaitAsync,

    /// <summary>
    /// <para>Move immediately to DfuState_Failed.</para>
    /// <para>The state handling function returns this when it detects
    /// a bad state, such as unexpected data from the attached board.</para>
    /// </summary>
    StateTransition_Failed,

    /// <summary>
    /// <para>This code is used by the state machine, rather than the state
    /// handling functions to indicate that the state machine should exit,
    /// successfully or otherwise.
    /// </summary>
    StateTransition_Done
} StateTransition;

/// <summary>
/// Because the state machine runs asynchronously, it must retain
/// its state while it is waiting to transition to the next state.
/// This structure holds that state.
/// </summary>
struct DeviceTransferState {
    /// <summary>
    /// The next state that MoveToNextDfuState will transition to.
    /// This is not the state which was just executed.
    /// </summary>
    DfuProtocolStates state;

    /// <summary>
    /// Data structure for init timer which is started after MT3620 resets the bootloader.
    /// If the file descriptor != -1, then the timer was also successfully
    /// added to epoll.
    /// </summary>
    EventData initTimerEventData;

    /// <summary>
    /// Data structure for post-validation timer which is started after
    /// a file has been written to the attached board.
    /// </summary>
    EventData postValidateTimerEventData;

    /// <summary>
    /// Holds up to one MTU worth of SLIP-encoded data which will be written
    /// to attached board.
    /// </summary>
    MemBuf *txBuf;

    /// <summary>
    /// Holds up to one MTU worth of data which has been received from attached board
    /// and SLIP-decoded.
    /// </summary>
    MemBuf *decodedRxBuf;

    /// <summary>
    /// Identifier sent with ping request. The state machine verifies that
    /// the ping response contains the same identifier.
    /// </summary>
    uint8_t pingId;

    /// <summary>Packet receipt notification. Always set to zero.</summary>
    uint16_t prn;

    /// <summary>Maximum transfer unit size in bytes.</summary>
    uint16_t mtu;

    /// <summary>
    /// Maximum number of bytes which can be sent in a single write command.
    /// This can be greater than the MTU size. This will differ according to
    /// whether sending init packet or firmware.
    /// </summary>
    uint32_t maxTxSize;

    /// <summary>CRC-32 of data which has been written so far.</summary>
    uint32_t runningCrc32;

    /// <summary>
    /// Provides access to the init packet file or the firmware file, whichever
    /// is currently being transferred.
    ///</summary>
    FileView *fv;

    /// <summary>
    /// Number of bytes to write in a single operation. This value is chosen so
    /// that the amount of data will not exceed the MTU size, even after SLIP encoding.
    /// </summary>
    off_t stepSize;

    /// <summary>
    /// A section of file is read into the file view. The data in that file view
    /// is itself divided into chunks which are sent one after another. This records
    /// how far into the file view (not the file) the data has been sent from.
    /// </summary>
    off_t offsetIntoFileView;

    /// <summary>
    /// How much data from the file view has been written to the attached board.
    /// This will be the lesser of stepSize and the remaining data in the file view.
    /// </summary>
    off_t fvFragmentLen;

    /// <summary>How many bytes have been written to the UART.</summary>
    size_t bytesSent;

    /// <summary>How many bytes have been read from the UART.</summary>
    size_t bytesRead;

    /// <summary>Whether to launch a read when the write completes successfully.</summary>
    bool readAfterWrite;

    /// <summary>
    /// How the SLIP decoding is progressing. Data is read from the UART one
    /// byte at a time and so need to keep track of whether in escape sequence.
    /// </summary>
    NrfSlipDecodeState decodeState;

    /// <summary>
    /// The functionality which sends the select (NrfDfuOp_ObjectSelect) request
    /// is re-used when sending the init packet and firmware data. The state
    /// machine transitions to this state when the response has been successfully
    /// received.
    /// </summary>
    DfuProtocolStates selectContinueState;

    /// <summary>
    /// The functionality which writes the file data to the attached board is
    /// re-used when sending the init packet and firmware data. The state machine
    /// transitions to this state when it has successfully written the file to
    /// the attached board.
    /// </summary>
    DfuProtocolStates fileTransferContinueState;

    /// <summary>
    /// Object passed back to timeout event handler.
    /// </summary>
    EventData timeoutTimerEventData;

    /// <summary>
    /// Whether waiting for an asynchronous read to complete on the UART.
    /// </summary>
    bool epollinEnabled;

    /// <summary>
    /// Whether waiting for an asynchronous write to complete on the UART.
    /// </summary>
    bool epolloutEnabled;
};

