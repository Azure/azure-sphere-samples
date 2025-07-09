// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices.WindowsRuntime;
    using System.Threading.Tasks;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.EventArgs;
    using Windows.Devices.Bluetooth;
    using Windows.Devices.Bluetooth.GenericAttributeProfile;
    using Windows.Devices.Enumeration;

    internal delegate void NotifyEventHandler(object sender, NotifyEventArgs e);

    internal sealed class BluetoothLeHelper
    {
        // Error codes
        private static readonly int E_BLUETOOTH_ATT_WRITE_NOT_PERMITTED = unchecked((int)0x80650003);
        private static readonly int E_BLUETOOTH_ATT_INVALID_PDU = unchecked((int)0x80650004);
        private static readonly int E_ACCESSDENIED = unchecked((int)0x80070005);

        private GattCharacteristic notificationCharacteristic;
        private bool isListening = false;

        public event NotifyEventHandler NotificationReceived;

        public static async Task WriteAsync(byte[] data, GattDeviceService service, Guid characteristicId)
        {
            if (data == null || data.Length == 0)
            {
                throw new InvalidOperationException("No data to write to device.");
            }

            try
            {
                // Writes the value from the buffer to the characteristic.
                Debug.WriteLine($"Getting Bluetooth LE characteristic to write to.");
                GattCharacteristic characteristic = await GetCharacteristicAsync(service, characteristicId);

                if (!characteristic.CharacteristicProperties.HasFlag(GattCharacteristicProperties.Write))
                {
                    throw new InvalidOperationException("This characteristic does not support writing.");
                }

                Debug.WriteLine($"Writing data to Bluetooth LE characteristic.");
                var result = await characteristic.WriteValueWithResultAsync(data.AsBuffer());

                if (result.Status != GattCommunicationStatus.Success)
                {
                    throw new InvalidOperationException($"Failed to write data: {result.Status}");
                }

                Debug.WriteLine($"Completed writing data to Bluetooth LE characteristic.");

            }
            catch (Exception ex) when (ex.HResult == E_BLUETOOTH_ATT_INVALID_PDU)
            {
                throw new InvalidOperationException("Failed to write data.", ex);
            }
            catch (Exception ex) when (ex.HResult == E_BLUETOOTH_ATT_WRITE_NOT_PERMITTED)
            {
                throw new InvalidOperationException("Not permitted to write data.", ex);
            }
            catch (Exception ex) when (ex.HResult == E_ACCESSDENIED)
            {
                throw new InvalidOperationException("Access denied.", ex);
            }
        }

        public async Task StartNotificationListenerAsync(GattDeviceService service, Guid characteristicId)
        {
            if (isListening) { return; }
            Debug.WriteLine($"Getting Bluetooth LE characteristic to receive notifications from.");
            notificationCharacteristic = await GetCharacteristicAsync(service, characteristicId);

            if (!notificationCharacteristic.CharacteristicProperties.HasFlag(GattCharacteristicProperties.Notify))
            {
                throw new InvalidOperationException("This characteristic does not support notifications.");
            }

            Debug.WriteLine($"Update Bluetooth LE characteristic to enable notification listener.");
            GattCommunicationStatus status = await notificationCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                        GattClientCharacteristicConfigurationDescriptorValue.Notify);
            if (status != GattCommunicationStatus.Success)
            {
                throw new InvalidOperationException("Unable to subscribe to notifications.");
            }

            notificationCharacteristic.ValueChanged += Characteristic_ValueChanged;
            isListening = true;
        }

        public async Task EndNotificationListenerAsync()
        {
            if (!isListening) { return; }
            Debug.WriteLine($"Update Bluetooth LE characteristic to disable notification listener.");
            GattCommunicationStatus status = await notificationCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                        GattClientCharacteristicConfigurationDescriptorValue.None);
            if (status != GattCommunicationStatus.Success)
            {
                throw new InvalidOperationException("Unable to unsubscribe from notifications.");
            }

            notificationCharacteristic.ValueChanged -= Characteristic_ValueChanged;
            isListening = false;
        }

        private void Characteristic_ValueChanged(GattCharacteristic sender, GattValueChangedEventArgs args)
        {
            if (sender == notificationCharacteristic)
            {
                Debug.WriteLine($"Received notification of data to Bluetooth LE characteristic.");
                NotificationReceived?.Invoke(this, new NotifyEventArgs(args.CharacteristicValue.ToArray()));
            }
        }

        private static async Task<GattCharacteristic> GetCharacteristicAsync(GattDeviceService service, Guid characteristicId)
        {
            if (service == null)
            {
                throw new InvalidOperationException("Failed to connect to service.");
            }

            IReadOnlyList<GattCharacteristic> characteristics = new List<GattCharacteristic>();

            try
            {
                // Ensure we have access to the device.
                var accessStatus = await service.RequestAccessAsync();
                if (accessStatus != DeviceAccessStatus.Allowed)
                {
                    // Not granted access
                    throw new InvalidOperationException($"Error accessing service: {accessStatus}");
                }

                // Get all the child characteristics of a service. Use the cache mode to specify uncached characteristics only 
                // and the new Async functions to get the characteristics of unpaired devices as well. 
                var result = await service.GetCharacteristicsAsync(BluetoothCacheMode.Uncached);
                if (result.Status != GattCommunicationStatus.Success)
                {
                    throw new InvalidOperationException($"Error accessing service: {result.Status}");
                }

                GattCharacteristic characteristic = result.Characteristics.FirstOrDefault(c => c.Uuid == characteristicId);

                if (characteristic == null)
                {
                    throw new InvalidOperationException($"Requested characteristic ID was not found.");
                }

                return characteristic;
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Restricted service. Can't read characteristics.", ex);
            }
        }
    }
}
