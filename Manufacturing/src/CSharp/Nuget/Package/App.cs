/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System;

    /// <summary>
    /// Device REST APIs to manage applications on the attached device.
    /// </summary>
    public static class App
    {
        /// <summary>Makes a "GET" request to retrieve the storage quota and usage for a specific component on an attached device.</summary>
        /// <param name="componentId"> The ID of the component to get the quota information for.</param>
        /// <returns>The storage quota as a string on success. An exception will be thrown on error.</returns>
        public static string GetAppQuota(string componentId)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetAppQuota", "3.1.0");
            if (string.IsNullOrEmpty(componentId) || !Validation.IsUuid(componentId))
            {
                throw new ValidationError("Cannot get app quota, invalid component ID!");
            }

            return RestUtils.GetRequest($"app/quota/{componentId}");
        }

        /// <summary>Makes a "GET" request to retrieve the status of a component from an attached device.</summary>
        /// <param name="componentID">The ID of the component to get the status for.</param>
        /// <returns>The application state as a string on success. An exception will be thrown on error.</returns>
        public static string GetAppStatus(string componentID)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetAppStatus", "2.0.0");
            if (string.IsNullOrEmpty(componentID) || !Validation.IsUuid(componentID))
            {
                throw new ValidationError("Cannot get app status, invalid component ID!");
            }

            return RestUtils.GetRequest($"app/status/{componentID}");
        }

        /// <summary>Makes a "GET" request to retrieve the memory statistics for applications on an attached device.</summary>
        /// <returns>The memory statistics for applications as a string on success. An exception will be thrown on error.</returns>
        public static string GetMemoryStatistics()
        {
            return RestUtils.GetRequest("stats/memory/groups/applications");
        }

        /// <summary>Makes a "PATCH" request to set the application status of a component. This requires enabling development mode.</summary>
        /// <param name="componentID">The ID of the component to get the quota information for.</param>
        /// <param name="state">The state of the application you would like to set.</param>
        /// <remarks>Accepted state values: ["start", "startDebug", "stop"]</remarks>
        /// <returns>
        /// The application state as a string on success. An exception will be thrown on error.
        /// </returns>
        public static string SetAppStatus(string componentID, string state)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("SetAppStatus", "2.0.0");
            if (string.IsNullOrEmpty(componentID) || !Validation.IsUuid(componentID))
            {
                throw new ValidationError("Cannot set app status, invalid componentID!");
            }

            string[] validTriggerValues = new string[] { "start", "startDebug", "stop" };

            if (string.IsNullOrEmpty(state) ||
                !Array.Exists(validTriggerValues, elem => elem.Equals(state)))
            {
                throw new ValidationError("Cannot set app status, invalid state!");
            }

            return RestUtils.PatchRequest($"app/status/{componentID}", new { trigger = state });
        }
    }

    /// <summary>Helper class, providing options for SetAppStatus</summary>
    public static class SetAppStatusTriggerOptions
    {
        /// <summary>start option</summary>
        public static string start = "start";
        /// <summary>startDebug option</summary>
        public static string startDebug = "startDebug";
        /// <summary>stop option</summary>
        public static string stop = "stop";
    }
}
