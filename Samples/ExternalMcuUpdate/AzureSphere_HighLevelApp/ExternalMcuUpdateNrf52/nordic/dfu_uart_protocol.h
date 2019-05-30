/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was done by Microsoft. See the
LICENSE.txt in this directory, and for more background, see the README.md for this sample. */

#pragma once

/// <summary>
/// These enums are equivalent with the ones used by the nRF52 bootloader to
/// check the firmware version.
/// </summary>
typedef enum {
    /// <summary>Softdevice firmware type.</summary>
    DfuFirmware_Softdevice = 0x00,

    /// <summary>Application firmware type.</summary>
    DfuFirmware_Application = 0x01,
} DfuFirmwareType;

/// <summary>
/// Each image, e.g. soft device or application uses two files, one for
/// the init packet, and one for the firmware, as well as a firmware type and
/// a version. This structure combines all of them into a single object.
/// </summary>
typedef struct {
    /// <summary>
    /// File containing init packet data.  The file must be included in
    /// the image package, and this path is relative to the image package root.
    /// </summary>
    const char *datPathname;

    /// <summary>
    /// File containing firmware data. The file must be included in
    /// the image package, and this path is relative to the image package root.
    /// </summary>
    const char *binPathname;

    /// <summary>Enum representing the firmware type to be updated.</summary>
    DfuFirmwareType firmwareType;

    /// <summary>Version of this firmware. It is only written to the 
    /// attached board if it is different from the version which is 
    /// already on the attached board.</summary>
    uint32_t version;

    /// <summary>Version of the firmware available on the attached board. 
    /// If the firmware is not present on the attached board, this field will
    /// have an undetermined value.</summary>
    uint32_t installedVersion;

    /// <summary>Whether an existing version of the image is present on the nRF52
    /// device.</summary>
    bool isInstalled;
} DfuImageData;

/// <summary>
/// Indicates whether the images were successfully written to the attached board.
/// </summary>
typedef enum {
    /// <summary>All images were written successfully.</summary>
    DfuResult_Success,

    /// <summary>
    /// All images were not written successfully.  A subset of the images
    /// may have been written.
    /// </summary>
    DfuResult_Fail
} DfuResultStatus;

/// <summary>
/// When the firmware update completes successfully or otherwise, it invokes
/// a callback of this type.
/// </summary>
typedef void (*DfuResultHandler)(DfuResultStatus statusToReturn);

/// <summary>
/// Supply opened file descriptors to the device firmware update protocol.
/// These resources must not be closed while the firmware is being updated.
/// The firmware update mechanism uses, but does not clean up these handles.
/// <param name="openedUartFd">Descriptor used to write to and read from attached board.</param>
/// <param name="openedResetFd">GPIO used to reset attached board.</param>
/// <param name="openedDfuFd">GPIO used to put attached board into DFU mode.</param>
/// <param name="openedEpollFd">Descriptor used to register for notifications when issuing
/// asynchronous reads and writes.</param>
/// </summary>
void InitUartProtocol(int openedUartFd, int openedResetFd, int openedDfuFd, int openedEpollFd);

/// <summary>
/// Start writing the supplied images to the attached board.  When the
/// images have been successfully written, or when the operation has failed,
/// the supplied exit handler will be called.
/// <param name="imagesToWrite">Array of images to write to the attached board.</param>
/// <param name="imageCount">Number of images in imagesToWrite array.</param>
/// <param name="exitHandler">Function to invoke when completed successfully or otherwise.</param>
/// </summary>
void ProgramImages(DfuImageData *imagesToWrite, size_t imageCount, DfuResultHandler exitHandler);

