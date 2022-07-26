/* Copyright (c) Microsoft Corporation. All rights reserved.
  Licensed under the MIT License. */

namespace Microsoft.Azure.Sphere.DeviceAPI.Sample
{
    using Microsoft.Azure.Sphere.DeviceAPI;
    using System.Text.Json;

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Azure Sphere Device API Sample.");
            Console.WriteLine("Gets a list of attached devices, displays the IP address and Device ID");
            Console.WriteLine();

            string result = Devices.GetAttachedDevices();
            List<DeviceInfo> devices = JsonSerializer.Deserialize<List<DeviceInfo>>(result);
            if (devices.Count > 0)
            {
                foreach (DeviceInfo device in devices)
                {
                    Devices.SetActiveDeviceIpAddress(device.IpAddress);
                    result = Device.GetDeviceSecurityState();
                    SecurityState state = JsonSerializer.Deserialize<SecurityState>(result);
                    Console.WriteLine($"{device.IpAddress}, {state.deviceIdentifier}");
                }
            }
            else
            {
                Console.WriteLine("No devices found.");
            }
        }
    }
    /// <summary>
    /// Class that contains result from Device.GetAttachedDevices()
    /// </summary>
    public class DeviceInfo
    {
        public string IpAddress { get; set; }
        public string DeviceConnectionPath { get; set; }
    }

    /// <summary>
    /// Class that contains result from Device.GetSecurityState()
    /// </summary>
    public class SecurityState
    {
        public string securityState { get; set; }
        public string deviceIdentifier { get; set; }
        public string deviceIdentityPublicKey { get; set; }
    }
}
