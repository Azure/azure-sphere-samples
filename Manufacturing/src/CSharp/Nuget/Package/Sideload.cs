/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System;
    using System.IO;

    /// <summary>
    /// Device REST APIs to deploy and manage applications on the attached device.
    /// </summary>
    public static class Sideload
    {
        /// <summary>Makes a "DELETE" request to remove a component from an attached device. This requires enabling development mode.</summary>
        /// <param name="componentID">The component id of the image to be deleted.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string DeleteImage(string componentID)
        {
            if (string.IsNullOrEmpty(componentID) || !Validation.IsUuid(componentID))
            {
                throw new ValidationError("Cannot delete image, invalid component ID.");
            }

            return RestUtils.DeleteRequest($"app/image/{componentID}");
        }

        /// <summary>Makes a "POST" request to install all staged images on an attached device. For deploying unsigned images, this requires enabling development mode.</summary>
        /// <param name="appControlMode">Determines if application starts automatically after installation, defaults to 'Auto'.</param>
        /// <remarks>Accepted trigger values: ["Auto", "Manual"]</remarks>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string InstallImages(string appControlMode = "Auto")
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("InstallImages", "3.0.0");
            string[] validAppControlModes = new string[] { "Auto", "Manual" };

            if (string.IsNullOrEmpty(appControlMode) || !Array.Exists(validAppControlModes, elem => elem.Equals(appControlMode)))
            {
                throw new ValidationError("Cannot install image, app control mode is invalid.");
            }

            return RestUtils.PostRequest("update/install", new { appControlMode });
        }

        /// <summary>Makes a "PUT" request to stage an image on an attached device. For deploying unsigned images, this requires enabling development mode.</summary>
        /// <param name="imageLocation">The file location of the image you would like to stage on an attached device.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string StageImage(string imageLocation)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("StageImage", "3.0.0");
            if (string.IsNullOrEmpty(imageLocation))
            {
                throw new ValidationError("Cannot stage image, image location is null or empty.");
            }

            if (!File.Exists(imageLocation))
            {
                throw new ValidationError("Cannot stage image, image location does not exist or is not accessible.");
            }

            byte[] imagePackageBytes = File.ReadAllBytes(imageLocation);

            return RestUtils.PutWithOctetStreamRequest("update/stage", imagePackageBytes);
        }
    }
}
