// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts
{
    using System;

    public abstract class RequestBase
    {
        internal RequestBase(WifiRequestId wifiRequestType, uint sequenceId, byte[] payload, int expectedPayloadLength)
        {
            if (payload == null && expectedPayloadLength != 0)
            {
                throw new ArgumentNullException(nameof(payload), "Payload should not be null.");
            }

            if (payload != null && payload.Length != expectedPayloadLength)
            {
                throw new ArgumentOutOfRangeException(nameof(payload), $"Payload should be {expectedPayloadLength} bytes. It is {payload.Length}.");
            }

            WifiRequestType = wifiRequestType;
            SequenceId      = sequenceId;
        }

        public WifiRequestId WifiRequestType { get; }

        public uint SequenceId { get; }
    }
}
