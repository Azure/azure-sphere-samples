// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    public sealed class DeviceControlReportLedStatusRequest : RequestBase
    {
        internal DeviceControlReportLedStatusRequest(DeviceControlRequestId deviceControlRequestId, uint sequenceId, byte[] payload)
            : base(CategoryIdType.DeviceControl, (ushort)deviceControlRequestId, sequenceId, payload, 4)
        {
            /* Data format:
             * 
             * - 00 [  1 ] LED Status
             * - 01 [  3 ] Reserved
             */
            this.LedStatus = (payload[0] == 0x01);
        }

        public bool LedStatus { get; }
    }
}
