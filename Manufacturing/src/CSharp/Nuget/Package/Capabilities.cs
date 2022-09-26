/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    /// <summary>
    /// Device  REST APIs to manage device capability configurations.
    /// </summary>
    public static class Capabilities
    {
        /// <summary>Makes a "GET" request to retrieve the current device capability configuration of the attached device.</summary>
        /// <returns>The device capability configuration as a string on success. An exception will be thrown on error.</returns>
        public static string GetDeviceCapabilities()
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetDeviceCapabilities");
            return RestUtils.GetRequest("device/capabilities");
        }
    }
}
