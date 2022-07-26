/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the get all configured wifi networks endpoint.
    /// </summary>
    [TestClass]
    public class GetAllConfiguredWifiNetworksTests
    {
        /// <summary>Removes any added wifi networks before each test.<summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanWifiNetworks();
        }

        /// <summary>Tests if getting all wifi networks before adding any returns an empty values list.<summary>
        [TestMethod]
        public void AllWifiNetworks_NotAdding_ReturnsEmptyList()
        {
            string response =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual("{\"values\":[]}", response);
        }

        /// <summary>Tests if getting all wifi networks after adding one shows the added network in the values list.<summary>
        [TestMethod]
        public void AllWifiNetworks_AddWifi_ReturnsNonEmptyList()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");

            string response =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual(
                "{\"values\":[{\"ssid\":\"Test\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false}]}",
                response);
        }

        /// <summary>Tests if adding multiple wifi networks then calling get all wifi networks shows all the wifi networks.<summary>
        [TestMethod]
        public void AllWifiNetworks_AddingMultiple_ReturnsMultipleInList()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");
            Wifi.AddWifiNetwork("Test2", "open", "", "enabled");

            string response =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual(
                "{\"values\":[{\"ssid\":\"Test\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false},{\"ssid\":\"Test2\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":1,\"securityState\":\"open\",\"targetedScan\":false}]}",
                response);
        }

        /// <summary>Tests if adding then deleting a wifi network shows the correct state at each stage.<summary>
        [TestMethod]
        public void AllWifiNetworks_AddThenDelete_ReturnsEmptyValuesList()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");

            string response =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual(
                "{\"values\":[{\"ssid\":\"Test\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false}]}",
                response);

            Wifi.DeleteWiFiNetConfig(0);

            string deletedResponse =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual("{\"values\":[]}", deletedResponse);
        }
    }
}
