// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    using System;
    using System.Text;

    public sealed class WifiGetNewDetailsResponse : ResponseBase
    {
        // SSID is defined by protocol as a byte array of 32 bytes or less.
        private const uint MaxSsidLength = 32;

        // A text based PSK is defined by protocol as a UTF8 string of 8 to 63 characters.
        // (NOTE: A binary PSK can be a byte array of up to 64 bytes, but this is not currently supported in this sample.)
        private const uint MinTextPskLength = 8;
        private const uint MaxTextPskLength = 63;

        public WifiGetNewDetailsResponse(byte[] ssid, SecurityType securityType, string psk, bool targetedScan)
        {
            if (ssid == null)
            {
                throw new ArgumentNullException(nameof(ssid), "SSID must not be null.");
            }

            if (ssid.Length > MaxSsidLength)
            {
                throw new ArgumentOutOfRangeException(nameof(ssid), "SSID must not be more than 32 bytes long.");
            }

            if (securityType == SecurityType.Unknown)
            {
                throw new ArgumentOutOfRangeException(nameof(securityType), $"Wi-Fi security type must not be '{SecurityType.Unknown.ToString()}'.");
            }

            if (securityType == SecurityType.WPA2)
            {
                if (string.IsNullOrWhiteSpace(psk))
                {
                    throw new ArgumentOutOfRangeException(nameof(psk), "If Wi-Fi network security is not open, you must provide a valid PSK.");
                }

                if (psk.Length < MinTextPskLength || psk.Length > MaxTextPskLength)
                {
                    throw new ArgumentOutOfRangeException(nameof(psk), "PSK must be between 8 and 63 characters.");
                }
            }

            Ssid         = ssid;
            SecurityType = securityType;
            Psk          = (SecurityType == SecurityType.WPA2) ? psk : null;
            TargetedScan = targetedScan;
        }

        public SecurityType SecurityType { get; }

        public byte[] Ssid { get; }

        public string Psk { get; }

        public bool TargetedScan { get; }

        internal override byte[] GetPayload()
        {
            // Create the byte representation of the PSK
            byte[] pskData = SecurityType == SecurityType.WPA2 ? Encoding.UTF8.GetBytes(Psk) : new byte[0];

            /* Data format:
             * 
             * - 00 [  1 ] Security type
             * - 01 [  1 ] SSID length
             * - 02 [  2 ] Reserved
             * - 04 [ 32 ] SSID
             * - 36 [  1 ] PSK length
             * - 37 [  3 ] Reserved
             * - 40 [ 64 ] PSK
             * - 104 [ 1 ] Targeted scan
             * - 105 [ 3 ] Reserved
             */

            byte[] payload = new byte[108];

            payload[0]  = (byte)SecurityType;
            payload[1]  = (byte)Ssid.Length;
            ByteArrayHelper.WriteBytes(Ssid, payload, 4);
            payload[36] = (byte)pskData.Length;
            ByteArrayHelper.WriteBytes(pskData, payload, 40);
            payload[104] = Convert.ToByte(TargetedScan);

            return payload;
        }
    }
}
