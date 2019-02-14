// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    public sealed class WifiSetRequest : RequestBase
    {
        internal WifiSetRequest(WifiRequestId wifiRequestType, uint sequenceId, byte[] payload)
            : base(CategoryIdType.WifiControl, (ushort)wifiRequestType, sequenceId, payload, 4)
        {
            /* Data format:
             * 
             * - 00 [  1 ] Operation result - 0x00 success, > 0x00 is error code
             * - 01 [  3 ] Reserved
             */

            this.ErrorCode = payload[0];
        }

        public byte ErrorCode { get; }
    }
}
