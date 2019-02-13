﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    using System;

    public sealed class WifiStatusRequest : RequestBase
    {
        internal WifiStatusRequest(WifiRequestId wifiRequestType, uint sequenceId, byte[] payload)
            : base(CategoryIdType.WifiControl, (ushort)wifiRequestType, sequenceId, payload, 48)
        {
            /* Data format:
             * 
             * - 00 [  1 ] WiFi connection status - 0x00 : WiFi connected, 0x01 : Internet connected
             * - 01 [  1 ] WiFi signal level - RSSI range from -128 to 0
             * - 02 [  1 ] Security type
             * - 03 [  1 ] SSID length
             * - 04 [ 32 ] SSID
             * - 36 [  4 ] WiFi frequency - UInt32 in MHz
             * - 40 [  6 ] BSSID
             * - 46 [  2 ] Reserved
             */

            IsWifiConnected     = (payload[0] & 0x01) == 0x01;
            IsInternetConnected = (payload[0] & 0x02) == 0x02;
            SignalStrength      = ByteArrayHelper.ReadSignedByte(payload, 1);
            SecurityType        = (SecurityType)payload[2];
            Ssid                = ByteArrayHelper.ReadBytes(payload, 4, payload[3]);
            FrequencyMhz        = ByteArrayHelper.ReadLsbUInt32(payload, 36);
            Bssid               = ByteArrayHelper.ReadDelimitedHex(payload, 40, 6, ':');
        }

        public bool IsWifiConnected { get; }

        public bool IsInternetConnected { get; }

        public short SignalStrength { get; }

        public SecurityType SecurityType { get; }

        public byte[] Ssid { get; }

        public uint FrequencyMhz { get; }

        public string Bssid { get; }
    }
}
