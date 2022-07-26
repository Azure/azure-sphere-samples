/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the change configured wifi endpoint.
    /// </summary>
    [TestClass]
    public class ChangeConfiguredWifiTests
    {
        /// <summary>Removes all added wifi network configurations before each test.<summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanWifiNetworks();
        }

        /// <summary>Tests if changing a configured network with a null config state throws a validation error.<summary>
        [TestMethod]
        public void ChangeNetwork_NullConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.ChangeConfiguredWifiNetwork(0, null, ""));
            Assert.AreEqual("Cannot change Wi-Fi network config, configState is invalid!", exception.Message);
        }

        /// <summary>Tests if changing a configured network with an empty config state throws a validation error.<summary>
        [TestMethod]
        public void ChangeNetwork_EmptyConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.ChangeConfiguredWifiNetwork(0, "", ""));
            Assert.AreEqual("Cannot change Wi-Fi network config, configState is invalid!", exception.Message);
        }

        /// <summary>Tests if changing a configured network with an invalid config state throws a validation error.<summary>
        [TestMethod]
        public void ChangeNetwork_InvalidConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.ChangeConfiguredWifiNetwork(0, "InvalidConfigState", ""));
            Assert.AreEqual("Cannot change Wi-Fi network config, configState is invalid!", exception.Message);
        }

        /// <summary>Tests if changing a configured network with a null psk state throws a validation error.<summary>
        [TestMethod]
        public void ChangeNetwork_NullPSK_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.ChangeConfiguredWifiNetwork(0, "unknown", null));
            Assert.AreEqual("Cannot change Wi-Fi network config, psk is invalid!", exception.Message);
        }

        /// <summary>Tests if changing a configured network with an empty psk throws a validation error.<summary>
        [TestMethod]
        public void ChangeNetwork_EmptyPSK_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.ChangeConfiguredWifiNetwork(0, "unknown", ""));
            Assert.AreEqual("Cannot change Wi-Fi network config, psk is invalid!", exception.Message);
        }

        /// <summary>Tests if changing a non existent network throws a device error.<summary>
        [TestMethod]
        public void ChangeNetwork_ChangeNonExistentNetwork_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Wifi.ChangeConfiguredWifiNetwork(0, "unknown", "TestNetwork"));
            Assert.AreEqual("This resource is unavailable on this device. Network not found", exception.Message);
        }

        /// <summary>Tests if changing an existing network returns the expected network configuration.<summary>
        [TestMethod]
        public void ChangeNetwork_ChangeExistingNetwork_ReturnsExpectedNetworkConfiguration()
        {
            Wifi.AddWifiNetwork("NETWORK1", "psk", configState
                                : "enabled", targetScan
                                : false, psk
                                : "EXAMPLEPSK");
            string response =
                Wifi.ChangeConfiguredWifiNetwork(0, "disabled", "NETWORK1");

            Assert.AreEqual(
                "{\"ssid\":\"NETWORK1\",\"configState\":\"disabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"psk\",\"targetedScan\":false}",
                response);
        }
    }
}
