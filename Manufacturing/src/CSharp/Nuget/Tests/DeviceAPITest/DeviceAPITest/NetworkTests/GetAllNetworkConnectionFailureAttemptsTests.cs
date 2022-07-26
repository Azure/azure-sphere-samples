/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get all network connection failure attempts api endpoint.
    /// </summary>
    [TestClass]
    public class GetAllNetworkConnectionFailureAttemptsTests
    {
        /// <summary>Tests if calling get all failed network connections returns an empty values list.</summary>
        [TestMethod]
        public void AllFailedConnections_Get_ReturnsEmptyValuesList()
        {
            string response =
                Network.GetAllFailedNetworkConnections();

            Assert.AreEqual("{\"values\":[]}", response);
        }
    }
}
