/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.FunctionalTests
{
    /// <summary>
    /// A test class for the get certificate api endpoint.
    /// </summary>
    [TestClass]
    public class OldDeviceAPIVersionExcepts
    {
        /// <summary>
        /// Sets the device api version to an old version before testing
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            SinceDeviceAPIVersion.SetDeviceApiVersion("1.0.0");
        }

        /// <summary>
        /// Attempts to make a call from an incompatible device API version.
        /// </summary>
        [TestMethod]
        public void OldDeviceAPIVersion_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Device.GetDeviceOSVersion());
            Assert.AreEqual(exception.Message, "The current device does not support GetDeviceOSVersion. Required DeviceAPI version: 4.5.0. DeviceAPI version reported by device: 1.0.0");
        }
    }
}
