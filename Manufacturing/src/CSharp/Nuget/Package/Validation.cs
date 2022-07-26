/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System;

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
}
