/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System;

    /// <summary>
    /// Device REST APIs to manage the manufacturing state of attached devices.
    /// </summary>
    public static class Manufacturing
    {
        /// <summary>Makes a REST "GET" request to retrieve the manufacturing state of the attached device.</summary>
        /// <returns>The manufacturing state of the device as a string on success. An exception will be thrown on error.</returns>
        public static string GetManufacturingState()
        {
            return RestUtils.GetRequest("device/manufacturing_state");
        }

        /// <summary>Makes a REST "PUT" request to update the manufacturing state of the attached device. This cannot be undone.</summary>
        /// <param name="manufacturingState">The manufacturing state of the attached device.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string SetDeviceManufacturingState(string manufacturingState)
        {
            string[] validSettableManufacturingStates = new string[] { "DeviceComplete", "Module1Complete" };

            if (string.IsNullOrEmpty(manufacturingState) || !Array.Exists(validSettableManufacturingStates, elem => elem.Equals(manufacturingState)))
            {
                throw new ValidationError("Cannot set manufacturing state, manufacturing state supplied is invalid.");
            }

            return RestUtils.PutRequest("device/manufacturing_state", new { manufacturingState });
        }
    }
}
