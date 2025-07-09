// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.EventArgs
{
    using System;

    internal sealed class NotifyEventArgs : EventArgs
    {
        public NotifyEventArgs(byte[] data)
        {
            Data = data;
        }

        public byte[] Data { get; }
    }
}
