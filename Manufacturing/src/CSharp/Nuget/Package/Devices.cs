/* Copyright (c) Microsoft Corporation. All rights reserved.
  Licensed under the MIT License. */

using System.Diagnostics;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

namespace Microsoft.Azure.Sphere.DeviceAPI
{
    /// <summary>
    /// REST APIs to get devices information.
    /// </summary>
    public static class Devices
    {
        /// <summary>
        /// Device IP used for all REST API calls
        /// </summary>
        private static string DeviceIP = "192.168.35.2";

        private static Regex IPRegEx = new Regex("192.168.35.\\b([2-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\b");

        /// <summary>
        /// Set the IP address used for REST calls
        /// </summary>
        /// <param name="IpAddress">Device IP Address used for REST API Calls</param>
        public static void SetActiveDeviceIpAddress(string IpAddress)
        {
            if (IPRegEx.IsMatch(IpAddress))
            {
                if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(OSPlatform.Linux) && IpAddress != "192.168.35.2")
                {
                    throw new ValidationError("ERROR: Cannot set active device IP address {ip_address} on Linux. Linux does not have multi-board support.");
                }

                DeviceIP = IpAddress;
            }
            else
            {
                throw new AzureSphereException("Cannot set active device IP address, range is 192.168.35.2-192.168.35.255");
            }
        }

        /// <summary>
        /// Get the IP address used for REST calls
        /// </summary>
        /// <returns>string containing the device REST API IP Address</returns>
        public static string GetActiveDeviceIpAddress()
        {
            return DeviceIP;
        }

        /// <summary>Makes a "GET" request to retrieve the attached devices.</summary>
        /// <returns>The list of attached devices as a string on success. An exception will be thrown on error.</returns>
        public static string GetAttachedDevices()
        {
            // Setup using localhost instead of ip address

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                string url = "http://localhost:48938/";
                return RestUtils.GetRequest("api/service/devices", url);
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                if (NetworkInterface.GetAllNetworkInterfaces()
                        .Where(iface => iface.Name.Equals("sl0")).Any())
                {
                    return $"[{{\"IpAddress\":\"{GetActiveDeviceIpAddress()}\",\"DeviceConnectionPath\":\"{string.Empty}\"}}]";
                }
                // No interfaces with sl0
                Debug.WriteLine("No devices found!");
                return "[]";
            }

            // Unsupported operating system
            throw new AzureSphereException("Cannot get the attached devices, unsupported operating system!");
        }
    }
}
