// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.EventArgs
{
    using System;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts;

    public sealed class DeviceControlLedStatusNeededEventArgs : EventArgs
    {
        public DeviceControlLedStatusNeededEventArgs(DeviceControlReportLedStatusRequest request)
        {
            Request = request;
        }

        public DeviceControlReportLedStatusRequest Request { get; }
    }
}
