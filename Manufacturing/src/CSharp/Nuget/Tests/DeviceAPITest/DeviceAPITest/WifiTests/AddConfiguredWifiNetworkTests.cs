/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the add configured wifi network endpoint.
    /// </summary>
    [TestClass]
    public class AddConfiguredWifiNetworkTests
    {
        /// <summary>Removes all added wifi network configurations before each test.<summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanWifiNetworks();
        }

        /// <summary>Tests if trying to add a wifi network with a null ssid throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_NullSsid_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork(null));
            Assert.AreEqual("Cannot add wifi network config, invalid ssid!", exception.Message);
        }

        /// <summary>Tests if trying to add a network with an empty ssid throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_EmptySsid_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork(""));
            Assert.AreEqual("Cannot add wifi network config, invalid ssid!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with a null config state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_NullConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("NotNullOrEmpty!", configState: null));
            Assert.AreEqual("Cannot add wifi network config, invalid configState!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with an empty config state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_EmptyConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("NotNullOrEmpty!", configState: ""));
            Assert.AreEqual("Cannot add wifi network config, invalid configState!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with an invalid config state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_InvalidConfigState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("NotNullOrEmpty!", configState: "InvalidState!"));
            Assert.AreEqual("Cannot add wifi network config, invalid configState!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with valid config state returns the expected values list.<summary>
        [TestMethod]
        public void AddWifiNetwork_ValidConfigStates_ReturnsExpectedValuesList()
        {
            string[] validConfigState = new string[] { "enabled", "disabled" };

            for (int i = 0; i < validConfigState.Length; i++)
            {
                string response = Wifi.AddWifiNetwork(
                    $"TestSsid{i}", "open", "example", validConfigState[i]);
            }

            string endResponse =
                Wifi.GetAllConfiguredWifiNetworks();

            Assert.AreEqual(
                "{\"values\":[{\"ssid\":\"TestSsid0\",\"configState\":\"enabled\",\"connectionState\":\"disconnected\",\"id\":0,\"securityState\":\"open\",\"targetedScan\":false,\"configName\":\"example\"},{\"ssid\":\"TestSsid1\",\"configState\":\"disabled\",\"connectionState\":\"disconnected\",\"id\":1,\"securityState\":\"open\",\"targetedScan\":false,\"configName\":\"example\"}]}",
                endResponse);
        }

        /// <summary>Tests if trying to add a wifi network with a null security state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_NullSecurityState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("TestSsid", null, "example", "enabled"));
            Assert.AreEqual("Cannot add wifi network config, invalid network type!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with an empty security state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_EmptySecurityState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("TestSsid", "", "example", "enabled"));
            Assert.AreEqual("Cannot add wifi network config, invalid network type!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with an invalid security state throws a validation error.<summary>
        [TestMethod]
        public void AddWifiNetwork_InvalidSecurityState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Wifi.AddWifiNetwork("TestSsid", "InvalidSecurityState!", "example", "enabled"));
            Assert.AreEqual("Cannot add wifi network config, invalid network type!", exception.Message);
        }

        /// <summary>Tests if trying to add a wifi network with valid security states returns the expected network configurations.<summary>
        [TestMethod]
        public void AddWifiNetwork_ValidSecurityState_ReturnsExpectedConfigurations()
        {
            string eaptls = "eaptls";
            string open = "open";
            string pskState = "psk";

            /// Eaptls
            string eaptlsResponse = Wifi.AddWifiNetwork(
                "TestEaptls", eaptls, "example1", "enabled", clientCertStoreIdentifier
                : "example");

            Assert.AreEqual(
                "{\"ssid\":\"TestEaptls\",\"configState\":\"enabled\",\"connectionState\":\"unknown\",\"id\":0,\"securityState\":\"eaptls\",\"targetedScan\":false,\"configName\":\"example1\",\"clientCertStoreIdentifier\":\"example\"}",
                eaptlsResponse);

            /// Open
            string openResponse =
                Wifi.AddWifiNetwork("TestOpen", open, "example2", "enabled");

            Assert.AreEqual(
                "{\"ssid\":\"TestOpen\",\"configState\":\"enabled\",\"connectionState\":\"unknown\",\"id\":1,\"securityState\":\"open\",\"targetedScan\":false,\"configName\":\"example2\"}",
                openResponse);

            /// psk
            string pskResponse =
                Wifi.AddWifiNetwork("NETWORK1", pskState, configState
                                    : "enabled", targetScan
                                    : false, psk
                                    : "EXAMPLEPSK");
            Assert.AreEqual(
                "{\"ssid\":\"NETWORK1\",\"configState\":\"enabled\",\"connectionState\":\"unknown\",\"id\":2,\"securityState\":\"psk\",\"targetedScan\":false}",
                pskResponse);
        }
    }
}
