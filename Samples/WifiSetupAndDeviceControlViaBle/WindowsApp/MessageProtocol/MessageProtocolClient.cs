// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol
{
    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.EventArgs;
    using Windows.Devices.Bluetooth;
    using Windows.Devices.Bluetooth.GenericAttributeProfile;
    using Windows.Devices.Enumeration;

    public delegate void WifiStatusRequestEventHandler(object sender, WifiStatusRequestEventArgs e);
    public delegate void WifiScanRequestEventHandler(object sender, WifiScanRequestEventArgs e);
    public delegate void WifiAddNetworkRequestEventHandler(object sender, WifiAddNetworkRequestEventArgs e);

    public delegate void ReportLedStatusRequestEventHandler(object sender, DeviceControlLedStatusNeededEventArgs e);

    public sealed class MessageProtocolClient
    {
        private static readonly int E_DEVICE_NOT_AVAILABLE = unchecked((int)0x800710df); // HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE)

        private static readonly Guid MessageProtocolServiceId = Guid.Parse("59140001-9252-f896-e811-7ab292fa018c");
        private static readonly Guid MessageProtocolRxCharacteristicId = Guid.Parse("59140002-9252-f896-e811-7ab292fa018c");
        private static readonly Guid MessageProtocolTxCharacteristicId = Guid.Parse("59140003-9252-f896-e811-7ab292fa018c");

        private BluetoothLeHelper bluetoothLeHelper = new BluetoothLeHelper();
        private GattDeviceService currentService;
        private uint actualWifiNetworkCount;
        private uint expectedWifiNetworkCount;
        private WifiGetNewDetailsResponse wifiGetNewDetailsResponse;
        private DeviceControlGetDesiredLedStatusResponse deviceControlGetDesiredLedStatusResponse;

        public event WifiStatusRequestEventHandler WifiStatusRequestReceived;
        public event WifiScanRequestEventHandler WifiNetworkScanReceived;
        public event WifiAddNetworkRequestEventHandler WifiAddNetworkStatusReceived;
        public event ReportLedStatusRequestEventHandler ReportLedStatusRequestReceived;

        ~MessageProtocolClient()
        {
            bluetoothLeHelper.EndNotificationListenerAsync().GetAwaiter().GetResult();
        }

        public static async Task<GattDeviceService> GetServiceAsync(string deviceId)
        {
            BluetoothLEDevice device = null;
            try
            {
                Debug.WriteLine("Connecting to Bluetooth LE device.");
                device = await BluetoothLEDevice.FromIdAsync(deviceId);
            }
            catch (Exception ex) when (ex.HResult == E_DEVICE_NOT_AVAILABLE)
            {
                throw new InvalidOperationException("Bluetooth radio is not on.", ex);
            }

            if (device == null)
            {
                throw new InvalidOperationException("Failed to connect to device.");
            }

            if (device.DeviceInformation.Pairing.CanPair && !device.DeviceInformation.Pairing.IsPaired)
            {
                DevicePairingResult dpr = await device.DeviceInformation.Pairing.PairAsync(DevicePairingProtectionLevel.EncryptionAndAuthentication);
                if (dpr == null)
                {
                    throw new InvalidOperationException("Failed to pair with device");
                }
                if (!(dpr.Status == DevicePairingResultStatus.Paired || dpr.Status == DevicePairingResultStatus.AlreadyPaired))
                {
                    throw new InvalidOperationException($"Failed to pair with device with result: '{dpr.Status}'");
                }

            }

            Debug.WriteLine("Requesting 'Wi-Fi config message protocol' service for device.");
            GattDeviceServicesResult result = await device.GetGattServicesAsync(BluetoothCacheMode.Uncached);

            if (result.Status != GattCommunicationStatus.Success)
            {
                throw new InvalidOperationException("Device unreachable.");
            }

            GattDeviceService service = result.Services.FirstOrDefault(s => s.Uuid == MessageProtocolServiceId);

            if (service == null)
            {
                throw new InvalidOperationException("This device does not support the Message Protocol.");
            }

            Debug.WriteLine("Connected to Bluetooth LE device.");
            return service;
        }

        public async Task ListenForWifiStatusAsync(GattDeviceService service)
        {
            Debug.WriteLine("Requesting Wi-Fi status.");
            currentService = service;

            // Request Wi-Fi status
            bluetoothLeHelper.NotificationReceived += WifiStatusRequest_NotificationReceived;
            await bluetoothLeHelper.StartNotificationListenerAsync(service, MessageProtocolTxCharacteristicId);
            await SendEventMessageAsync(service, CategoryIdType.WifiControl, (ushort)WifiEventId.WifiStatusNeeded);
        }

        public void ListenForReportedLedStatusAsync()
        {
            bluetoothLeHelper.NotificationReceived += DeviceControlReportLedStatusRequest_NotificationReceived;
        }

        public async Task RequestReporLedStatusAsync(GattDeviceService service)
        {
            Debug.WriteLine("Requesting LED status.");
            currentService = service;

            // Request LED status
            await bluetoothLeHelper.StartNotificationListenerAsync(service, MessageProtocolTxCharacteristicId);
            await SendEventMessageAsync(service, CategoryIdType.DeviceControl, (ushort)DeviceControlEventId.LedStatusNeeded);
        }

        public async Task ListenForVisibleWifiNetworksAsync(GattDeviceService service)
        {
            Debug.WriteLine("Requesting visible Wi-Fi networks.");
            currentService = service;

            // Request Wi-Fi Scan
            bluetoothLeHelper.NotificationReceived += WifiScanSummaryRequest_NotificationReceived;
            await bluetoothLeHelper.StartNotificationListenerAsync(service, MessageProtocolTxCharacteristicId);
            await SendEventMessageAsync(service, CategoryIdType.WifiControl, (ushort)WifiEventId.WifiScanNeeded);
        }

        public async Task AddWifiNetworkAsync(GattDeviceService service, byte[] ssid, SecurityType securityType, string psk, bool targetedScan)
        {
            Debug.WriteLine("Adding new Wi-Fi network.");
            currentService = service;

            // Set the details of the new Wi-Fi network
            wifiGetNewDetailsResponse = new WifiGetNewDetailsResponse(ssid, securityType, psk, targetedScan);

            // Set new Wi-Fi network
            bluetoothLeHelper.NotificationReceived += WifiGetNewDetailsRequest_NotificationReceived;
            await bluetoothLeHelper.StartNotificationListenerAsync(service, MessageProtocolTxCharacteristicId);
            await SendEventMessageAsync(service, CategoryIdType.WifiControl, (ushort)WifiEventId.NewWifiDetailsAvailable);
        }

        public async Task SetDesiredLedStatusAsync(GattDeviceService service, bool ledStatus)
        {
            Debug.WriteLine("Setting desired LED status: %d.", ledStatus);
            currentService = service;

            // Set the desired LED status
            deviceControlGetDesiredLedStatusResponse = new DeviceControlGetDesiredLedStatusResponse(ledStatus);

            // Set desired LED status request notification
            bluetoothLeHelper.NotificationReceived += DeviceControlGetDesiredLedStatusRequest_NotificationReceived;
            await bluetoothLeHelper.StartNotificationListenerAsync(service, MessageProtocolTxCharacteristicId);
            await SendEventMessageAsync(service, CategoryIdType.DeviceControl, (ushort)DeviceControlEventId.DesiredLedStatusAvailable);
        }

        private async void WifiStatusRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is WifiStatusRequest wifiStatusRequest)
            {
                Debug.WriteLine($"Received Wi-Fi config message protocol request: '{wifiStatusRequest.RequestType}'");

                bluetoothLeHelper.NotificationReceived -= WifiStatusRequest_NotificationReceived;
                await SendResponseAsync(currentService, wifiStatusRequest, 0x00);

                WifiStatusRequestReceived?.Invoke(this, new WifiStatusRequestEventArgs(wifiStatusRequest));
            }
        }

        private async void WifiScanSummaryRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is WifiScanSummaryRequest wifiScanSummaryRequest)
            {
                Debug.WriteLine($"Received Wi-Fi config message protocol request: '{wifiScanSummaryRequest.RequestType}'");

                bluetoothLeHelper.NotificationReceived -= WifiScanSummaryRequest_NotificationReceived;

                if (wifiScanSummaryRequest.ErrorCode != 0x00)
                {
                    throw new InvalidOperationException($"Wi-Fi network scan failed with error code {wifiScanSummaryRequest.ErrorCode}");
                }

                if (wifiScanSummaryRequest.NetworkCount == 0)
                {
                    // Report that we're not getting anything.
                    WifiNetworkScanReceived?.Invoke(this, new WifiScanRequestEventArgs(null, 0, 0));
                }
                else
                {
                    bluetoothLeHelper.NotificationReceived += WifiScanResultRequest_NotificationReceived;
                    await bluetoothLeHelper.StartNotificationListenerAsync(currentService, MessageProtocolTxCharacteristicId);

                    actualWifiNetworkCount = 0;
                    expectedWifiNetworkCount = wifiScanSummaryRequest.NetworkCount;

                    await SendResponseAsync(currentService, wifiScanSummaryRequest, wifiScanSummaryRequest.ErrorCode);
                }
            }
        }

        private async void WifiScanResultRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is WifiScanResultRequest wifiScanResultRequest)
            {
                Debug.WriteLine($"Received Wi-Fi config message protocol request: '{wifiScanResultRequest.RequestType}'");

                actualWifiNetworkCount++;
                WifiNetworkScanReceived?.Invoke(this, new WifiScanRequestEventArgs(wifiScanResultRequest, actualWifiNetworkCount, expectedWifiNetworkCount));

                await SendResponseAsync(currentService, wifiScanResultRequest, 0x00);

                if (actualWifiNetworkCount == expectedWifiNetworkCount)
                {
                    bluetoothLeHelper.NotificationReceived -= WifiScanResultRequest_NotificationReceived;
                }
            }
        }

        private async void WifiGetNewDetailsRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is WifiGetNewDetailsRequest wifiGetNewDetailsRequest)
            {
                Debug.WriteLine($"Received Wi-Fi config message protocol request: '{wifiGetNewDetailsRequest.RequestType}'");

                bluetoothLeHelper.NotificationReceived -= WifiGetNewDetailsRequest_NotificationReceived;

                // Setup success status check with device
                bluetoothLeHelper.NotificationReceived += WifiSetRequest_NotificationReceived;
                await bluetoothLeHelper.StartNotificationListenerAsync(currentService, MessageProtocolTxCharacteristicId);

                // Send the add network request.
                await SendResponseAsync(currentService, wifiGetNewDetailsRequest, 0x00, wifiGetNewDetailsResponse);
            }
        }

        private async void WifiSetRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is WifiSetRequest wifiSetRequest)
            {
                Debug.WriteLine($"Received Wi-Fi config message protocol request: '{wifiSetRequest.RequestType}'");

                bluetoothLeHelper.NotificationReceived -= WifiSetRequest_NotificationReceived;

                // Send the response
                await SendResponseAsync(currentService, wifiSetRequest, wifiSetRequest.ErrorCode);

                WifiAddNetworkStatusReceived?.Invoke(this, new WifiAddNetworkRequestEventArgs(wifiSetRequest.ErrorCode));
            }
        }

        private async void DeviceControlGetDesiredLedStatusRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is DeviceControlGetDesiredLedStatusRequest deviceControlGetDesiredLedStatusRequest)
            {
                Debug.WriteLine($"Received Device Control message protocol request: '{deviceControlGetDesiredLedStatusRequest.RequestType}'");

                bluetoothLeHelper.NotificationReceived -= DeviceControlGetDesiredLedStatusRequest_NotificationReceived;
                
                // Send the get desired LED status response.
                await SendResponseAsync(currentService, deviceControlGetDesiredLedStatusRequest, 0x00, deviceControlGetDesiredLedStatusResponse);
            }
        }

        private async void DeviceControlReportLedStatusRequest_NotificationReceived(object sender, NotifyEventArgs e)
        {
            if (MessageProtocolFactory.ReadRequestMessagePayload(e.Data) is DeviceControlReportLedStatusRequest deviceControlReportLedStatusRequest)
            {
                Debug.WriteLine($"Received Device Control message protocol request: '{deviceControlReportLedStatusRequest.RequestType}'");

                // Need to listen to DeviceControlUpdateLedStatusRequest all the time.

                // Send the report LED status response.
                await SendResponseAsync(currentService, deviceControlReportLedStatusRequest, 0x00);

                // Need to call back to DevicePage.xaml.cs to update Toggle Switch's value & enabled.
                ReportLedStatusRequestReceived?.Invoke(this, new DeviceControlLedStatusNeededEventArgs(deviceControlReportLedStatusRequest));
            }
        }
        
        private static async Task SendEventMessageAsync(GattDeviceService service, CategoryIdType categoryId, ushort eventType)
        {
            Debug.WriteLine($"Sending message protocol event: '{categoryId}, {eventType}'");

            byte[] eventMessage = MessageProtocolFactory.CreateEventMessage(categoryId, eventType);
            await BluetoothLeHelper.WriteAsync(eventMessage, service, MessageProtocolRxCharacteristicId);
        }

        private static async Task SendResponseAsync(GattDeviceService service, RequestBase request, byte errorCode, ResponseBase response = null)
        {
            Debug.WriteLine($"Sending message protocol response: '{request.CategoryId}, {request.RequestType}'");

            byte[] responseMessage = MessageProtocolFactory.CreateResponseMessage(request.CategoryId, request.RequestType, request.SequenceId, errorCode, response);
            await BluetoothLeHelper.WriteAsync(responseMessage, service, MessageProtocolRxCharacteristicId);
        }
    }
}
