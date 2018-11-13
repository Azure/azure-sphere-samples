// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.EventArgs
{
    using System;

    public sealed class WifiAddNetworkRequestEventArgs : EventArgs
    {
        public WifiAddNetworkRequestEventArgs(byte errorCode)
        {
            ErrorCode = errorCode;
        }

        public byte ErrorCode { get; }
    }
}
