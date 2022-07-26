/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get network status endpoint.
    /// </summary>
    [TestClass]
    public class GetNetworkStatusTests
    {
        /// <summary>Tests if getting the network status returns the expected values.</summary>
        [TestMethod]
        public void NetworkStatus_Get_ReturnsExpectedStatus()
        {
            string response = Network.GetNetworkStatus();

            //Assert.AreEqual(HttpStatusCode.OK, code);
            Assert.AreEqual(
                "{\"deviceAuthenticationIsReady\":false,\"networkTimeSync\":\"incomplete\",\"proxy\":\"disabled\"}",
                response);
        }
    }
}
