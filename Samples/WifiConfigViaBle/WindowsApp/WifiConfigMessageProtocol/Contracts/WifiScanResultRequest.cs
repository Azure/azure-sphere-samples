// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts
{
    using System;
    using System.Text;

    public sealed class WifiScanResultRequest : RequestBase
    {
        internal WifiScanResultRequest(WifiRequestId wifiRequestType, uint sequenceId, byte[] payload)
            : base(wifiRequestType, sequenceId, payload, 36)
        {
            /* Data format:
             * 
             * - 00 [  1 ] Security type
             * - 01 [  1 ] WiFi signal level - RSSI range from -128 to 0
             * - 02 [  1 ] SSID length
             * - 03 [  1 ] Reserved
             * - 04 [ 32 ] SSID
             */

            SecurityType   = (SecurityType)payload[0];
            SignalStrength = ByteArrayHelper.ReadSignedByte(payload, 1);
            Ssid           = ByteArrayHelper.ReadBytes(payload, 4, payload[2]);
        }

        public SecurityType SecurityType { get; }

        public short SignalStrength { get; }

        public byte[] Ssid { get; }

        public override string ToString()
        {
            return $"{Encoding.UTF8.GetString(Ssid)} - {SecurityType.ToString()} ({SignalStrength}dB)";
        }

    }
}
