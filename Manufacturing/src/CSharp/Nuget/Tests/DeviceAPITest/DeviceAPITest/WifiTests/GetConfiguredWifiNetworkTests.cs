/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the get configured wifi network endpoint.
    /// </summary>
    [TestClass]
    public class GetConfiguredWifiNetworkTests
    {
        /// <summary>Removes any added wifi networks before each test.<summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanWifiNetworks();
        }

        /// <summary>Tests if getting a wifi network with a non existent id throws a device error.<summary>
        [TestMethod]
        public void GetWifiNetwork_NonExistentId_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Wifi.GetConfiguredWifiNetwork(0));
            Assert.AreEqual("This resource is unavailable on this device. Network not found", exception.Message);

        }

        /// <summary>Tests if getting a network with an existing id returns the expected network configuration.<summary>
        [TestMethod]
        public void GetWifiNetwork_ExistingId_ReturnsExpectedConfiguration()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");

            string response = Wifi.GetConfiguredWifiNetwork(0);

            Assert.AreEqual(
                "{\"ssid\":\"Test\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false}",
                response);
        }
    }
}
