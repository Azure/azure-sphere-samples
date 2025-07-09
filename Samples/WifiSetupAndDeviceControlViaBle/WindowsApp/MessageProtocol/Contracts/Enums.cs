// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts
{
    internal enum MessageType
    {
        Request  = 0x01,
        Response = 0x02,
        Event    = 0x03
    }

    public enum CategoryIdType
    {
        WifiControl     = 0x0002,
        DeviceControl   = 0x0003
    }

    public enum WifiEventId : ushort
    {
        NewWifiDetailsAvailable  = 0x0001,
        WifiStatusNeeded         = 0x0002,
        WifiScanNeeded           = 0x0003
    }

    public enum DeviceControlEventId : ushort
    {
        DesiredLedStatusAvailable   = 0x0001,
        LedStatusNeeded             = 0x0002
    }

    public enum WifiRequestId : ushort
    {
        GetNewWifiDetails         = 0x0001,
        SetWifiScanResultsSummary = 0x0002,
        SetWifiStatus             = 0x0003,
        SetWifiOperationResult    = 0x0004,
        SetNextWifiScanResult     = 0x0005
    }

    public enum DeviceControlRequestId : ushort
    {
        GetDesiredLedStatus     = 0x0001,
        ReportLedStatus         = 0x0002
    }

    public enum SecurityType
    {
        Unknown = 0x00,
        Open    = 0x01,
        WPA2    = 0x02
    }

    // Potential error codes returned by the Azure Sphere OS on the device when adding a new Wi-Fi network to the device.
    public enum WifiAddNetworkRequestErrorCode
    {
        Success       = 0,
        NetworkExists = 17
    }
}
