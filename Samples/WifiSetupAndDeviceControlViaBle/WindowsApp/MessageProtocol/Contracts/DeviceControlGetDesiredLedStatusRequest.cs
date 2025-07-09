// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    public sealed class DeviceControlGetDesiredLedStatusRequest : RequestBase
    {
        internal DeviceControlGetDesiredLedStatusRequest(DeviceControlRequestId deviceControlRequestId, uint sequenceId)
            : base(CategoryIdType.DeviceControl, (ushort)deviceControlRequestId, sequenceId, null, 0)
        {
            // This request type doesn't have a payload.
        }
    }
}
