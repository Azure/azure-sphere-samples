/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    /// <summary>
    /// Device REST APIs to manage device images.
    /// </summary>
    public static class Image
    {
        /// <summary>Makes a REST "GET" request to retrieve a list of images on an attached device.</summary>
        /// <returns>All images running on the device as a string on success. An exception will be thrown on error.</returns>
        public static string GetImages()
        {
            return RestUtils.GetRequest("images");
        }
    }
}
