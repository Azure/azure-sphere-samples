/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.SideloadTests
{
    /// <summary>
    /// A test class for the install images api endpoint.
    /// </summary>
    [TestClass]
    public class InstallImagesTests
    {
        /// <summary>
        /// Removes all application images that arent gdb server.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanImages();
        }

        /// <summary>
        /// Tests if installing images returns an empty json response.
        /// </summary>
        [TestMethod]
        public void InstallImages_Call_ReturnEmptyJsonResponse()
        {
            string response = Sideload.InstallImages();

            Assert.AreEqual("{}", response);
        }

        /// <summary>
        /// Tests if a newly installed image is running.
        /// </summary>
        [TestMethod]
        public void InstallImages_InstalledComponent_ReturnsOkIsRunning()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string response =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", response);
        }

        /// <summary>
        /// Tests if installing images changes the state of the image.
        /// </summary>
        [TestMethod]
        public void
        InstallImages_InstallingImagesChangesState_ReturnsIsRunning()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            string blinkResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"notPresent\"}", blinkResponse);

            Sideload.InstallImages();

            string installedResponse =
                App.GetAppStatus(Utilities.blinkComponentId);

            Assert.AreEqual("{\"state\":\"running\"}", installedResponse);
        }
    }
}
