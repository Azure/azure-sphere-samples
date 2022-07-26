/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the remove configured wifi network endpoint.
    /// </summary>
    [TestClass]
    public class RemoveConfiguredWifiNetworkTests
    {
        /// <summary>Tests if get wifi scan returns wifi networks of the expected format.<summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanWifiNetworks();
        }

        /// <summary>Tests if removing a network that doesnt exist throws a device error.<summary>
        [TestMethod]
        public void RemoveNetwork_NoNetwork_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Wifi.DeleteWiFiNetConfig(0));
            Assert.AreEqual("This resource is unavailable on this device. Network not found", exception.Message);
        }

        /// <summary>Tests if removing an added network removes the network.<summary>
        [TestMethod]
        public void RemoveNetwork_RemoveAddedNetwork_RemovesNetwork()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");

            string startResponse =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual(
                "{\"values\":[{\"ssid\":\"Test\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false}]}",
                startResponse);

            string response = Wifi.DeleteWiFiNetConfig(0);

            Assert.AreEqual("{}", response);

            string endResponse =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual("{\"values\":[]}", endResponse);
        }

        /// <summary>Tests if removing an added network returns an empty json response.<summary>
        [TestMethod]
        public void RemoveNetwork_RemoveAddedNetwork_ReturnsEmptyJsonResponse()
        {
            Wifi.AddWifiNetwork("Test", "open", "", "enabled");

            string response = Wifi.DeleteWiFiNetConfig(0);

            Assert.AreEqual("{}", response);
        }
    }
}
