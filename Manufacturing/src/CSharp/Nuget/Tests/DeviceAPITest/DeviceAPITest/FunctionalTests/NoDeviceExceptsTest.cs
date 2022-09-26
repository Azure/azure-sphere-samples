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
    public class NoDeviceExcepts
    {
        /// <summary>
        /// Sets the active IP address to be an invalid address before testing
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Devices.SetActiveDeviceIpAddress("192.168.35.51");
        }

        /// <summary>
        /// 
        /// </summary>
        [TestMethod]
        public void NoDevice_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Device.GetDeviceStatus());
            Assert.AreEqual(exception.Message, "Device connection timed out for 192.168.35.51. Please ensure your device is connected and has development mode enabled");
        }

        /// <summary>
        /// Returns the active ip address to its default
        /// </summary>
        [TestCleanup]
        public void TestCleanup()
        {
            Devices.SetActiveDeviceIpAddress("192.168.35.2");
        }
    }
}
