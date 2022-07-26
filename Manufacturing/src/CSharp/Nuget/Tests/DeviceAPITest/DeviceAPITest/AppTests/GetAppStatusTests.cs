/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.AppTests
{
    /// <summary>
    /// A test class for the get manufacturing state api endpoint.
    /// </summary>
    [TestClass]
    public class GetAppStatusTests
    {
        /// <summary>
        /// Removes all application images that are not gdb server.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanImages();
        }

        /// <summary>
        /// Tests if getting the app status with a null component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_WhenComponentIdIsNull_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppStatus(null));
            Assert.AreEqual("Cannot get app status, invalid component ID!", exception.Message);
        }

        /// <summary>
        /// Tests if getting the app status with an empty component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_WhenComponentIdIsEmpty_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppStatus(""));
            Assert.AreEqual("Cannot get app status, invalid component ID!", exception.Message);
        }

        /// <summary>
        /// Tests if getting the app statis with a non uuid format component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_WhenComponentIdIsNotUuidFormat_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppStatus("1234"));
            Assert.AreEqual("Cannot get app status, invalid component ID!", exception.Message);
        }


        /// <summary>
        /// Tests if getting the app statis when component id is valid uuid but doesnt exist returns a not present response.
        /// </summary>
        [TestMethod]
        public void
        GetAppStatus_WhenComponentIdIsUuidButDoesntExist_ReturnsNotPresent()
        {
            string response = App.GetAppStatus(Utilities.randomUUID);

            Assert.AreEqual("{\"state\":\"notPresent\"}", response);
        }

        /// <summary>
        /// Tests if getting the app statis when component id exists returns is running.
        /// </summary>
        [TestMethod]
        public void
        GetAppStatus_WhenComponentIdIsUuidFormatAndExists_ReturnsIsRunning()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string response = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", response);
        }

        /// <summary>
        /// Tests if getting the app status of a newlyt staged component returns not present.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_NewlyStagedComponent_ReturnsNotPresent()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);

            string response = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", response);
        }

        /// <summary>
        /// Tests if getting the app status of a newly deleted component returns not present.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_NewlyDeletedComponent_ReturnsNotPresent()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            Sideload.DeleteImage(Utilities.blinkComponentId);

            string response = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", response);
        }

        /// <summary>
        /// Tests if getting the app status of a newly installed component returns is running.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_NewlyInstalledComponent_ReturnsIsRunning()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string response = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", response);
        }

        /// <summary>
        /// Tests if installing an image is reflected in getting the app status.
        /// </summary>
        [TestMethod]
        public void GetAppStatus_InstallingImagesChangesState_ReturnsIsRunning()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            string blinkResponse = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", blinkResponse);

            Sideload.InstallImages();

            string installedResponse = App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", installedResponse);
        }
    }
}
