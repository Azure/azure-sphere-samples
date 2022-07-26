/* Copyright (c) Microsoft Corporation. All rights reserved.
  Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System.Diagnostics;
    using System.Net.NetworkInformation;
    using System.Runtime.InteropServices;
    using System.Text.Json;
    using System.Linq;
    using System.Net;
    using System.Collections.Generic;

    /// <summary>
    /// Device REST APIs to get device information.
    /// </summary>
    public static class Device
    {
        /// <summary>Makes a "DELETE" request to clear a device's error report data.</summary>
        /// <returns>An empty response on success. An exception will be thrown on error.</returns>
        public static string ClearErrorReportData()
        {
            return RestUtils.DeleteRequest("telemetry");
        }


        /// <summary>Makes a "GET" request to get the device rest api version.</summary>
        /// <returns>The api version number as a string on success. An exception will be thrown on error.</returns>
        public static string GetDeviceRestAPIVersion()
        {
            return RestUtils.GetRequest("status", headers: true);
        }

        /// <summary>Makes a "GET" request to retrieve the security state of an attached device.</summary>
        /// <returns>The security state as a string on success. An exception will be thrown on error.</returns>
        public static string GetDeviceSecurityState()
        {
            return RestUtils.GetRequest("device/security_state");
        }

        /// <summary>Makes a "GET" request to retrieve the uptime of a device.</summary>
        /// <returns>The device status as a string on success. An exception will be thrown on error.</returns>
        public static string GetDeviceStatus()
        {
            return RestUtils.GetRequest("status");
        }

        /// <summary>Makes a "GET" request to retrieve diagnostics logs on an attached device.</summary>
        /// <returns>The diagnostics log binary as a string on success. An exception will be thrown on error.</returns>
        public static string GetDiagnosticLog()
        {
            return RestUtils.GetRequest("log");
        }

        /// <summary>Makes a "GET" request to get a device's error report data.</summary>
        /// <returns>The device error report data binary as a string on success. An exception will be thrown on error.</returns>
        public static string GetErrorReportData()
        {
            return RestUtils.GetRequest("telemetry");
        }

        /// <summary>Makes a "POST" request to restart an attached device.</summary>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string RestartDevice()
        {
            return RestUtils.PostRequestNoBody("restart");
        }
    }
}
