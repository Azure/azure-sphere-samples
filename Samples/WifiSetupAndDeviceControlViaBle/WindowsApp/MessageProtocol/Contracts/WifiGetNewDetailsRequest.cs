// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    public sealed class WifiGetNewDetailsRequest : RequestBase
    {
        internal WifiGetNewDetailsRequest(WifiRequestId wifiRequestType, uint sequenceId)
            : base(CategoryIdType.WifiControl, (ushort)wifiRequestType, sequenceId, null, 0)
        {
            // This request type doesn't have a payload.
        }
    }
}
