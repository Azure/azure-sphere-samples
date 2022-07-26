/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the delete network proxy api endpoint.
    /// </summary>
    [TestClass]
    public class DeleteNetworkProxyTests
    {
        /// <summary>Tests if attempting to delete a proxy when there isn't one, returns an empty response.</summary>
        [TestMethod]
        public void DeleteProxy_DeleteWhenNoProxy_ReturnsEmptyResponse()
        {
            string response = Network.DeleteNetworkProxy();

            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if calling delete when there is a proxy, returns an empty response.</summary>
        [TestMethod]
        public void DeleteProxy_DeleteWhenProxy_ReturnsEmptyResponse()
        {
            Network.ConfigureProxy(true, "example.com", 8081, new List<string>(),
                                   "anonymous");

            string response = Network.DeleteNetworkProxy();

            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if attempting to delete a proxy when there is a proxy, deletes the proxy.</summary>
        [TestMethod]
        public void DeleteProxy_DeleteProxy_DeletesProxy()
        {
            Network.ConfigureProxy(true, "example.com", 8081, new List<string>(),
                                   "anonymous");

            string startResponse =
                Network.GetNetworkProxy();

            Assert.AreEqual(
                "{\"address\":\"example.com\",\"enabled\":true,\"port\":8081,\"authenticationType\":\"anonymous\",\"noProxyAddresses\":[]}",
                startResponse);

            Network.DeleteNetworkProxy();

            string response = Network.GetNetworkProxy();
            Assert.AreEqual("{}", response);
        }
    }
}
