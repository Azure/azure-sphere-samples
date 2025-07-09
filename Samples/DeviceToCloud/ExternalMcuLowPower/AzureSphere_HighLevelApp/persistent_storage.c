/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <applibs/storage.h>
#include <applibs/log.h>

#include "telemetry.h"
#include "persistent_storage.h"

static const uint32_t magicWord0 = ('M' << 24) | ('S' << 16) | ('A' << 8) | 'S';
static const uint32_t magicWord1 = ('S' << 24) | ('O' << 16) | ('D' << 8) | 'A';

bool PersistentStorage_RetrieveTelemetry(DeviceTelemetry *telemetry)
{
    int storageFd = -1;

    if (telemetry == NULL) {
        Log_Debug("ERROR: Telemetry pointer cannot be NULL\n");
        goto fail;
    }

    memset(telemetry, 0, sizeof(DeviceTelemetry));

    storageFd = Storage_OpenMutableFile();
    if (storageFd == -1) {
        Log_Debug("ERROR: Failed to open mutable storage - %s (%d)\n", strerror(errno), errno);
        goto fail;
    }

    uint32_t header[3];
    ssize_t bytesRead = read(storageFd, &header[0], sizeof(header));
    if (bytesRead == -1) {
        Log_Debug("ERROR: Failed to read telemetry header from mutable storage - %s (%d)\n",
                  strerror(errno), errno);
        goto fail;
    }

    if (bytesRead < sizeof(header)) {
        Log_Debug(
            "Failed to read telemetry header from mutable storage: incomplete header (%d bytes "
            "when %u were expected)\n",
            bytesRead, sizeof(header));
        goto fail;
    }

    if (header[0] != magicWord0 || header[1] != magicWord1) {
        Log_Debug(
            "Mutable storage does not contain header bytes; no stored telemetry available.\n");
        goto fail;
    }

    uint32_t persistedTelemetryVersion = header[2];
    if (persistedTelemetryVersion != telemetryStructVersion) {
        Log_Debug(
            "Persisted telemetry struct version (%d) differs from expected version (%d); no stored "
            "telemetry available",
            persistedTelemetryVersion, telemetryStructVersion);
        goto fail;
    }

    bytesRead = read(storageFd, telemetry, sizeof(DeviceTelemetry));

    if (bytesRead == -1) {
        Log_Debug("ERROR: Failed to read telemetry from mutable storage - %s (%d)", strerror(errno),
                  errno);
        goto fail;
    }

    if (bytesRead < sizeof(DeviceTelemetry)) {
        Log_Debug(
            "ERROR: Failed to read full telemetry struct from mutable storage; no stored telemetry "
            "available");
        goto fail;
    }

    return true;

fail:

    if (storageFd != 0) {
        close(storageFd);
    }
    return false;
}

void PersistentStorage_PersistTelemetry(const DeviceTelemetry *telemetry)
{
    int storageFd = -1;

    if (telemetry == NULL) {
        Log_Debug("ERROR: Telemetry pointer cannot be NULL\n");
        goto cleanup;
    }

    storageFd = Storage_OpenMutableFile();
    if (storageFd == -1) {
        Log_Debug("ERROR: Failed to open mutable storage - %s (%d)\n", strerror(errno), errno);
        goto cleanup;
    }

    uint32_t header[] = {magicWord0, magicWord1, telemetryStructVersion};
    ssize_t bytesWritten = write(storageFd, &header, sizeof(header));
    if (bytesWritten == -1) {
        Log_Debug("ERROR: Failed to write telemetry header to persistent storage - %s (%d)\n",
                  strerror(errno), errno);
        goto cleanup;
    }

    if (bytesWritten < sizeof(header)) {
        Log_Debug(
            "ERROR: Failed to write full telemetry header to persistent storage - only wrote %d of "
            "%u bytes\n",
            bytesWritten, sizeof(header));
        goto cleanup;
    }

    bytesWritten = write(storageFd, telemetry, sizeof(DeviceTelemetry));
    if (bytesWritten == -1) {
        Log_Debug("ERROR: Failed to write telemetry to persistent storage - %s (%d)\n",
                  strerror(errno), errno);
        goto cleanup;
    }

    if (bytesWritten < sizeof(DeviceTelemetry)) {
        Log_Debug(
            "ERROR: Failed to write full telemetry to persistent storage - only wrote %d of %u "
            "bytes\n",
            bytesWritten, sizeof(DeviceTelemetry));
        goto cleanup;
    }

cleanup:
    if (storageFd != 0) {
        close(storageFd);
    }
}
