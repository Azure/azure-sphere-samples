/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.SideloadTests
{
    /// <summary>
    /// A test class for the stage image api endpoint.
    /// </summary>
    [TestClass]
    public class StageImageTests
    {
        /// <summary>
        /// Removes all application images that aren't gdb server.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanImages();
        }

        /// <summary>
        /// Tests if staging an image with a null image location throws validation error.
        /// </summary>
        [TestMethod]
        public void StageImage_NullImageLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.StageImage(null));
            Assert.AreEqual("Cannot stage image, image location is null or empty.", exception.Message);
        }

        /// <summary>
        /// Tests if staging an image with an empty string location throws validation error.
        /// </summary>
        [TestMethod]
        public void StageImage_EmptyImageLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.StageImage(""));
            Assert.AreEqual("Cannot stage image, image location is null or empty.", exception.Message);
        }

        /// <summary>
        /// Tests if staging an image with a non file location throws validation error.
        /// </summary>
        [TestMethod]
        public void StageImage_NonFileLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Sideload.StageImage("NotAFileLocation!"));
            Assert.AreEqual("Cannot stage image, image location does not exist or is not accessible.", exception.Message);
        }

        /// <summary>
        /// Tests if staging an image with a valid file path returns an empty json response.
        /// </summary>
        [TestMethod]
        public void StageImage_FileLocation_ReturnsEmptyJsonResponse()
        {
            string response =
                Sideload.StageImage(Utilities.pathToBlinkImage);

            Assert.AreEqual("{}", response);
        }

        /// <summary>
        /// Tests if staging an image doesnt deploy the image.
        /// </summary>
        [TestMethod]
        public void StageImage_StageValidImage_DoesntDeployImage()
        {
            string startResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", startResponse);

            Sideload.StageImage(Utilities.pathToBlinkImage);

            string endResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", endResponse);
        }

        /// <summary>
        /// Tests if deploying an image changes its state to running.
        /// </summary>
        [TestMethod]
        public void StageImage_StagingImage_AllowsInstallation()
        {
            Sideload.InstallImages();
            string startResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", startResponse);

            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string endResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", endResponse);
        }
    }
}
