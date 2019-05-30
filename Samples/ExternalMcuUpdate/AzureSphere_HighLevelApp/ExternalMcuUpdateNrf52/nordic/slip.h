/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was done by Microsoft. See the
LICENSE.txt in this directory, and for more background, see the README.md for this sample. */

#pragma once

#include "../mem_buf.h"

/// <summary>
/// SLIP special character codes, as defined by RFC 1055.
/// </summary>
typedef enum {
    /// <summary>End of SLIP packet.</summary>
    NRF_SLIP_BYTE_END = 0300,

    /// <summary>The following character is escaped.</summary>
    NRF_SLIP_BYTE_ESC = 0333,

    /// <summary>Escaped END character, must follow ESC.</summary>
    NRF_SLIP_BYTE_ESC_END = 0334,

    /// <summary>Escaped ESC character, must follow ESC.</summary>
    NRF_SLIP_BYTE_ESC_ESC = 0335
} NrfSlipEscapeCodes;

/// <summary>State of SLIP decoding state machine.</summary>
typedef enum {
    /// <summary>Processing non-escaped character.</summary>
    NRF_SLIP_STATE_DECODING = 1,

    /// <summary>Previous character was ESC.</summary>
    NRF_SLIP_STATE_ESC_RECEIVED = 2,

    /// <summary>Invalid state - unexpected escaped character.</summary>
    NRF_SLIP_STATE_CLEARING_INVALID_PACKET = 3
} NrfSlipDecodeState;

/// <summary>
/// Append multiple bytes to the SLIP-encoded buffer.
/// <param name="encBuf">Buffer which contains SLIP-encoded data.</param>
/// <param name="data">Start of data to encode and append to buffer.</param>
/// <param name="len">Length of unencoded data in bytes.</param>
/// </summary>
void SlipEncodeAppend(MemBuf *encBuf, const uint8_t *data, size_t len);

/// <summary>
/// Append an end-of-packet marker to the SLIP-encoded buffer.
/// <param name="encBuf">Buffer which contains SLIP-encoded data.</param>
/// </summary>
void SlipEncodeAddEndMarker(MemBuf *encBuf);

/// <summary>
/// Process a single SLIP-encoded byte and add it to the buffer which
/// contains decoded data.
/// <param name="b">Encoded byte to process.</param>
/// <param name="decBuf">Buffer which contains decoded data.</param>
/// <param name="state">Keeps track of whether in escaped sequence or
/// processing invalid data.</param>
/// <param name="finished">Set to true if reached end of packet, false otherwise.</param>
/// </summary>
void SlipDecodeAddByte(uint8_t b, MemBuf *decBuf, NrfSlipDecodeState *state, bool *finished);
