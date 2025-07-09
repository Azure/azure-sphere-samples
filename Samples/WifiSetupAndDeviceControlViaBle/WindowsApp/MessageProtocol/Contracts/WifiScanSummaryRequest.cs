// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    public sealed class WifiScanSummaryRequest : RequestBase
    {
        internal WifiScanSummaryRequest(WifiRequestId wifiRequestType, uint sequenceId, byte[] payload)
            : base(CategoryIdType.WifiControl, (ushort)wifiRequestType, sequenceId, payload, 8)
        {
            /* Data format:
             * 
             * - 00 [  1 ] Scan result - 0x00 success, > 0x00 is error code
             * - 01 [  1 ] Total network count
             * - 02 [  2 ] Reserved
             * - 04 [  4 ] Total results size - the number of bytes needed to store scan results
             */

            ErrorCode    = payload[0];
            NetworkCount = payload[1];
        }

        public byte ErrorCode { get; }

        public uint NetworkCount { get; }
    }
}
