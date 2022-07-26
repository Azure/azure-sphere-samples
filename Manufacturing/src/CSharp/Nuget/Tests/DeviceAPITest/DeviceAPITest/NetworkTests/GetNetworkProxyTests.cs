/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get network proxy endpoint.
    /// </summary>
    [TestClass]
    public class GetNetworkProxyTests
    {

        /// <summary>Removes any added network proxies before each test.</summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Network.DeleteNetworkProxy();
        }

        /// <summary>Tests if getting the network proxy when there isnt onem returns an empty json response.</summary>
        [TestMethod]
        public void NetworkProxy_ReturnsEmptyJsonResponse()
        {
            string response = Network.GetNetworkProxy();

            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if configuring a network proxy shows the proxy when attempting to get it.</summary>
        [TestMethod]
        public void NetworkProxy_ConfigureProxy_ShowsProxyInGet()
        {
            Network.ConfigureProxy(true, "example.com", 8081, new List<string>(),
                                   "anonymous");

            string response = Network.GetNetworkProxy();

            Assert.AreEqual(
                "{\"address\":\"example.com\",\"enabled\":true,\"port\":8081,\"authenticationType\":\"anonymous\",\"noProxyAddresses\":[]}",
                response);
        }

        /// <summary>Tests if deleting a configured network proxy and then deleting it, doesnt then show up when attempting to get it.</summary>
        [TestMethod]
        public void NetworkProxy_DeleteConfiguredProxy_ShowsNoProxyInGet()
        {
            Network.ConfigureProxy(true, "example.com", 8081, new List<string>(),
                                   "anonymous");
            Network.DeleteNetworkProxy();
            string response = Network.GetNetworkProxy();

            Assert.AreEqual("{}", response);
        }
    }
}
