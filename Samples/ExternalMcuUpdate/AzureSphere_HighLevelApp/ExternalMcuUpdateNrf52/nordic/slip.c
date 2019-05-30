/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was done by Microsoft. See the
LICENSE.txt in this directory, and for more background, see the README.md for this sample. */

#include <assert.h>

#include "slip.h"

void SlipEncodeAppend(MemBuf *encBuf, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        uint8_t elem = data[i];

        if (elem == NRF_SLIP_BYTE_END) {
            MemBufAppend8(encBuf, NRF_SLIP_BYTE_ESC);
            MemBufAppend8(encBuf, NRF_SLIP_BYTE_ESC_END);
        } else if (elem == NRF_SLIP_BYTE_ESC) {
            MemBufAppend8(encBuf, NRF_SLIP_BYTE_ESC);
            MemBufAppend8(encBuf, NRF_SLIP_BYTE_ESC_ESC);
        } else {
            MemBufAppend8(encBuf, elem);
        }
    }
}

void SlipEncodeAddEndMarker(MemBuf *encBuf)
{
    MemBufAppend8(encBuf, NRF_SLIP_BYTE_END);
}

void SlipDecodeAddByte(uint8_t b, MemBuf *decBuf, NrfSlipDecodeState *state, bool *finished)
{
    *finished = false;
    switch (*state) {
    case NRF_SLIP_STATE_DECODING:
        if (b == NRF_SLIP_BYTE_END) {
            *finished = true;
        } else if (b == NRF_SLIP_BYTE_ESC) {
            *state = NRF_SLIP_STATE_ESC_RECEIVED;
        } else {
            MemBufAppend8(decBuf, b);
        }
        break;

    case NRF_SLIP_STATE_ESC_RECEIVED:
        if (b == NRF_SLIP_BYTE_ESC_END) {
            MemBufAppend8(decBuf, NRF_SLIP_BYTE_END);
            *state = NRF_SLIP_STATE_DECODING;
        } else if (b == NRF_SLIP_BYTE_ESC_ESC) {
            MemBufAppend8(decBuf, NRF_SLIP_BYTE_ESC);
            *state = NRF_SLIP_STATE_DECODING;
        } else { // unexpected data following escape
            *state = NRF_SLIP_STATE_CLEARING_INVALID_PACKET;
        }
        break;

    case NRF_SLIP_STATE_CLEARING_INVALID_PACKET:
        if (b == NRF_SLIP_BYTE_END) {
            *state = NRF_SLIP_STATE_DECODING;
            MemBufReset(decBuf);
        }
        break;

    default:
        assert(false);
        break;
    }
}
