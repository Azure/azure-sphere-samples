/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get device rest api version api endpoint.
    /// </summary>
    [TestClass]
    public class GetDeviceRestAPIVersionTests
    {
        /// <summary>
        /// Tests if getting the device rest api version returns the expected version in the expected format.
        /// </summary>
        [TestMethod]
        public void GetDeviceRestAPIVersion_Get_ReturnsExpectedVersion()
        {
            string response = Device.GetDeviceRestAPIVersion();

            Assert.AreEqual("{\"REST-API-Version\":\"4.4.0\"}", response);
        }
    }
}
