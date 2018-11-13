// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.EventArgs
{
    using System;
    using Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts;

    public sealed class WifiScanRequestEventArgs : EventArgs
    {
        public WifiScanRequestEventArgs(WifiScanResultRequest network, uint index, uint networkCount)
        {
            Network = network;
            Index = index;
            NetworkCount = networkCount;
        }

        public WifiScanResultRequest Network { get; }

        public uint Index { get; }

        public uint NetworkCount { get; }
    }
}
