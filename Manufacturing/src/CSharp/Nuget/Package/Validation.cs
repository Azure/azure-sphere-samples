/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;

    /// <summary>
    /// Holds methods used for validating inputs 
    /// </summary>
    public static class Validation
    {
        /// <summary>Validates if a given string represents a valid UUID.</summary>
        /// <param name="uuid">String being validated against being a valid UUID format.</param>
        /// <returns>True if string is valid UUID, or False otherwise</returns>
        public static bool IsUuid(string uuid)
        {
            return Guid.TryParse(uuid, out _);
        }
    }

    /// <summary>
    /// Validates invoked methods are applicable to the current DeviceAPI version.
    /// </summary>
    public class SinceDeviceAPIVersion
    {
        private static Dictionary<string, string> _versions = new Dictionary<string, string>
        {
            { "GetAppQuota", "3.1.0" },
            { "GetAppStatus", "2.0.0" },
            { "SetAppStatus", "2.0.0" },
            { "GetDeviceCapabilities", "3.1.0" },
            { "GetDeviceOSVersion", "4.5.0" },
            { "GetDiagnosticLog", "1.2.0" },
            { "GetErrorReportData", "3.1.0" },
            { "GetManufacturingState", "3.1.0" },
            { "SetDeviceManufacturingState", "3.1.0" },
            { "GetAllNetworkInterfaces", "3.1.0" },
            { "GetNetworkFirewallRuleset", "4.1.0" },
            { "GetNetworkInterface", "3.1.0" },
            { "GetNetworkStatus", "3.1.0" },
            { "SetNetworkInterfaces", "3.1.0" },
            { "InstallImages", "3.0.0" },
            { "StageImage", "3.0.0" },
            { "DeleteWiFiNetConfig", "3.0.0" }
        };

        private static string apiVersion = "";
        
        /// <summary>
        /// Internally this class stores the DeviceAPI version in use.
        /// This call resets the internal apiVersion of the device in use and should be called whenever a new device is connected.
        /// </summary>
        public static void SetDeviceApiVersion(string newApiVersion)
        {
            apiVersion = newApiVersion;
        }

        /// <summary>
        /// This method will compare the given methodName against the internal _versions dictionary.
        /// If there is a match, further validation will be performed between the DeviceAPI version reported by the device
        /// and the version number when the API was introduced.
        /// </summary>
        public static void ValidateDeviceApiVersion(string methodName)
        {
            if (apiVersion == "")
            {
                // a call to device rest api version updates the internal state of this class automatically
                Device.GetDeviceRestAPIVersion();
            }

            if (_versions.ContainsKey(methodName))
            {
                string sinceVersion = _versions.GetValueOrDefault(methodName);

                if (SemanticVersion.Parse(sinceVersion) > SemanticVersion.Parse(apiVersion))
                {
                    throw new DeviceError(string.Format("The current device does not support {0}. Required DeviceAPI version: {1}. DeviceAPI version reported by device: {2}", methodName, sinceVersion, apiVersion));
                }
            }
        }
    }
}
