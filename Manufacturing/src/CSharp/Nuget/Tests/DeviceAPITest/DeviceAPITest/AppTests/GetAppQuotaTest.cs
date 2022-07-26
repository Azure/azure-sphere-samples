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
    public class GetAppQuotaTest
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
        /// Tests if getting the app quota with a null component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_WhenComponentIdIsNull_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppQuota(null));
            Assert.AreEqual(exception.Message, "Cannot get app quota, invalid component ID!");
        }

        /// <summary>
        /// Tests if getting the app quota with an empty component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_WhenComponentIdIsEmpty_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppQuota(""));
            Assert.AreEqual(exception.Message, "Cannot get app quota, invalid component ID!");
        }

        /// <summary>
        /// Tests if getting the app quota with an invalid component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_WhenComponentIdIsNotUuidFormat_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.GetAppQuota("1234"));
            Assert.AreEqual(exception.Message, "Cannot get app quota, invalid component ID!");
        }

        /// <summary>
        /// Tests if getting the app quota with a valid but non existent component id throws a device error.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_WhenComponentIdIsUuidButDoesntExist_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => App.GetAppQuota(Utilities.randomUUID));
            Assert.AreEqual(exception.Message, "This resource is unavailable on this device. Application is not present");
        }

        /// <summary>
        /// Tests if getting the app quota with an existing component id returns the expected values for usage and limit.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_WhenComponentIdIsUuidFormatAndExists_ReturnsExpectedUsageLimit()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string uuid = Utilities.blinkComponentId;

            string response = App.GetAppQuota(uuid);

            Assert.AreEqual("{\"UsageKB\":0,\"LimitKB\":0}", response);
        }

        /// <summary>
        /// Tests if getting the app quota with the component id of a newly deleted app throws a device error.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_NewlyDeletedComponentAppQuota_ThrowsDeviceError()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            Sideload.DeleteImage(Utilities.blinkComponentId);

            DeviceError exception = Assert.ThrowsException<DeviceError>(() => App.GetAppQuota(Utilities.blinkComponentId));
            Assert.AreEqual("This resource is unavailable on this device. Application is not present", exception.Message);

        }

        /// <summary>
        /// Tests if deploying an image with mutable storage modifies the limit.
        /// </summary>
        [TestMethod]
        public void GetAppQuota_StageMutableStorageApp_IncreasesValue()
        {
            Sideload.StageImage(Utilities.pathToMutableStorage);

            Sideload.InstallImages();

            string blinkResponse = App.GetAppQuota(Utilities.mutableStorageId);

            Assert.AreEqual("{\"UsageKB\":0,\"LimitKB\":8}", blinkResponse);
        }
    }
}