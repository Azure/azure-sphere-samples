/* This code is a C port of the nrfutil Python tool from Nordic Semiconductor ASA. The porting was done by Microsoft. See the
LICENSE.txt in this directory, and for more background, see the README.md for this sample. */

#pragma once

#include <stdint.h>
#include <sys/types.h>

/// <summary>
/// Calculates the CRC-32 checksum for the supplied data.
/// <param name="data">Block of data over which to calculate checksum.</param>
/// <param name="len">Number of bytes in data block.</param>
/// <returns>The 32-bit checksum for the supplied data block.</returns>
/// <seealso cref="CalcCrc32WithSeed" />
/// </summary>
uint32_t CalcCrc32(const uint8_t *data, size_t len);

/// <summary>
/// <para>Calculates the CRC-32 checksum for the supplied data, given a seed.</para>
/// <para>Use this function to calculate the checksum for a large block of
/// data, where the data cannot be passed all at once to CalcCrc32.</para>
/// </summary>
/// <param name="data">Sub-block of data over which to calculate checksum.</param>
/// <param name="len">Number of bytes in sub-block.</param>
/// <param name="seed">Zero for the first sub-block, and the most recent
/// value returned by this function for subsequent blocks.</param>
/// <returns>The 32-bit checksum for the whole data block up to the end of the supplied block.
/// To calculate the checksum for subsequent parts of the data block, call this function
/// again, passing in the returned value as the seed.</returns>
/// <seealso cref="CalcCrc32" />
uint32_t CalcCrc32WithSeed(const uint8_t *data, size_t len, uint32_t seed);
