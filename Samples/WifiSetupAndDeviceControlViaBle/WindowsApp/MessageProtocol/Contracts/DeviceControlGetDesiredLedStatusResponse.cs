// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    using System;

    public sealed class DeviceControlGetDesiredLedStatusResponse : ResponseBase
    {
        public DeviceControlGetDesiredLedStatusResponse(bool ledStatus)
        {
            LedStatus = ledStatus;
        }

        public bool LedStatus { get; }

        internal override byte[] GetPayload()
        {
            /* Data format:
             * 
             * - 00 [  1 ] LED Status
             * - 01 [  3 ] Reserved
             */

            byte[] payload = new byte[4];

            payload[0]  = (byte)(LedStatus ? 0x01 : 0x00);

            return payload;
        }
    }
}
