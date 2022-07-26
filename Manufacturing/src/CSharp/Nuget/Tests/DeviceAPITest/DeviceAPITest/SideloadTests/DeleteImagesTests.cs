/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.SideloadTests
{
    /// <summary>
    /// A test class for the delete images api endpoint.
    /// </summary>
    [TestClass]
    public class DeleteImagesTests
    {
        /// <summary>
        /// Removes all application images except gdb server.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanImages();
        }

        /// <summary>
        /// Tests if deleting an image with a null component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void DeleteImage_NullComponentId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.DeleteImage(null));
            Assert.AreEqual("Cannot delete image, invalid component ID.", exception.Message);
        }

        /// <summary>
        /// Tests if deleting an image with an empty string component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void DeleteImage_EmptyComponentId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.DeleteImage(""));
            Assert.AreEqual("Cannot delete image, invalid component ID.", exception.Message);
        }

        /// <summary>
        /// Tests if deleting an image with a non existant component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void DeleteImage_NonUuidComponentId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.DeleteImage("InvalidUuid"));
            Assert.AreEqual("Cannot delete image, invalid component ID.", exception.Message);
        }

        /// <summary>
        /// Tests if deleting a valid component id returns an empty json response.
        /// </summary>
        [TestMethod]
        public void DeleteImage_DeleteNonExistentImage_ReturnsEmptyJsonResponse()
        {
            string response = Sideload.DeleteImage(Utilities.blinkComponentId);

            Assert.AreEqual("{}", response);
        }

        /// <summary>
        /// Tests if deleting an existing image's component id returns an empty response.
        /// </summary>
        [TestMethod]
        public void DeleteImage_DeleteExistingImage_ReturnsEmptyJsonResponse()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string response = Sideload.DeleteImage(Utilities.blinkComponentId);

            Assert.AreEqual("{}", response);
        }

        /// <summary>
        /// Tests if deleting an image is reflected in the current devices image components.
        /// </summary>
        [TestMethod]
        public void DeleteImage_DeleteImage_DeletesImageFromComponentsList()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            List<Dictionary<string, object>> images = Utilities.GetImageComponents();
            int startRunningApps = images.FindAll(elem => (long?)elem["image_type"] == 10).Count;

            Assert.IsTrue(
                images.Find(elem => ((string)elem["uid"]).Equals(Utilities.blinkComponentId)) != null);

            Sideload.DeleteImage(Utilities.blinkComponentId);

            List<Dictionary<string, object>> endImages =
                Utilities.GetImageComponents();

            int endRunningApps = endImages.FindAll(elem => (long?)elem["image_type"] == 10).Count;

            Dictionary<string, object> obs = endImages.Find(
                elem =>
                    ((string)elem["uid"]).Equals(Utilities.blinkComponentId));

            Assert.IsTrue(obs == null);
            Assert.IsTrue(startRunningApps > endRunningApps);
        }
    }
}
