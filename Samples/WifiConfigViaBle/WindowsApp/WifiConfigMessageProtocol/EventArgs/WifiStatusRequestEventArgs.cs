// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.EventArgs
{
    using System;
    using Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts;

    public sealed class WifiStatusRequestEventArgs : EventArgs
    {
        public WifiStatusRequestEventArgs(WifiStatusRequest request)
        {
            Request = request;
        }

        public WifiStatusRequest Request { get; }
    }
}
