/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the change wifi interface state endpoint.
    /// </summary>
    [TestClass]
    public class ChangeWifiInterfaceStateTests
    {
        /// <summary>Tests if changing the interface state to true returns an empty json response.<summary>
        [TestMethod]
        public void ChangeInterfaceState_GetTrue_ReturnsEmptyJsonResponse()
        {
            string response = Wifi.ChangeWiFiInterfaceState(true);
            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if changing the inteface state to true returns an empty json response.<summary>
        [TestMethod]
        public void ChangeInterfaceState_GetFalse_ReturnsEmptyJsonResponse()
        {
            string response = Wifi.ChangeWiFiInterfaceState(false);
            Assert.AreEqual("{}", response);
        }
    }
}
