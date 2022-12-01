/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using NuGet.Versioning;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the change wifi interface state endpoint.
    /// </summary>
    [TestClass]
    public class ChangeWifiInterfaceStateTests
    {
        /// <summary>Tests if setting reloadConfig to true returns an empty json response.<summary>
        [TestMethod]
        public void ChangeInterfaceState_ReloadCfgTrue_ReturnsEmptyJsonResponse()
        {
            string response = Wifi.ChangeWiFiInterfaceState(true, false);
            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if setting reloadConfig to false returns an empty json response.<summary>
        [TestMethod]
        public void ChangeInterfaceState_PowerSavingsFalse_ReturnsEmptyJsonResponse()
        {
            string response = Wifi.ChangeWiFiInterfaceState(false, false);
            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if setting powerSavings to true returns an empty json response (or asserts on older devices).<summary>
        [TestMethod]
        public void ChangeInterfaceState_PowerSavingsTrue_ReturnsEmptyJsonResponse()
        {
            if (SemanticVersion.Parse(SinceDeviceAPIVersion.GetDeviceApiVersion()) >= SemanticVersion.Parse("4.6.0"))
            {
                string response = Wifi.ChangeWiFiInterfaceState(false, true);
                Assert.AreEqual("{}", response);
            }
            else
            {
                Assert.ThrowsException<DeviceError>(() => Wifi.ChangeWiFiInterfaceState(false, true));
            }
        }

        /// <summary>Tests if setting powerSavings and reloadConfig to true returns an empty json response (or asserts on older devices).<summary>
        [TestMethod]
        public void ChangeInterfaceState_AllTrue_ReturnsEmptyJsonResponse()
        {
            if (SemanticVersion.Parse(SinceDeviceAPIVersion.GetDeviceApiVersion()) >= SemanticVersion.Parse("4.6.0"))
            {
                string response = Wifi.ChangeWiFiInterfaceState(true, true);
                Assert.AreEqual("{}", response);
            }
            else
            {
                Assert.ThrowsException<DeviceError>(() => Wifi.ChangeWiFiInterfaceState(true, true));
            }
        }

        /// <summary>Tests if setting powerSavings and reloadConfig to false returns an empty json respons.<summary>
        [TestMethod]
        public void ChangeInterfaceState_AllFalse_ReturnsEmptyJsonResponse()
        {
            string response = Wifi.ChangeWiFiInterfaceState(false, false);
            Assert.AreEqual("{}", response);
        }

        /// <summary>Tests if enabling power savings is successful or asserts (depending on the api version).<summary>
        [TestMethod]
        public void Check_SetWifiInterfacePowerSavings_Functions()
        {
            if (SemanticVersion.Parse(SinceDeviceAPIVersion.GetDeviceApiVersion()) >= SemanticVersion.Parse("4.6.0"))
            {
                Wifi.SetWiFiInterfacePowerSavings(true);
                JObject obj = JObject.Parse(Wifi.GetWiFiInterfaceState());
                Assert.IsTrue(obj["powerSavingsState"].ToString() == "enabled");
                Wifi.SetWiFiInterfacePowerSavings(false);
                obj = JObject.Parse(Wifi.GetWiFiInterfaceState());
                Assert.IsTrue(obj["powerSavingsState"].ToString() == "disabled");
            }
            else
            {
                Assert.ThrowsException<DeviceError>(() => Wifi.SetWiFiInterfacePowerSavings(true));
            }
        }

        /// <summary>Tests if enabling power savings via ChangeWiFiInterfaceState is successful or asserts (depending on the api version).<summary>
        [TestMethod]
        public void Check_ChangeWiFiInterfaceState_Functions()
        {
            if (SemanticVersion.Parse(SinceDeviceAPIVersion.GetDeviceApiVersion()) >= SemanticVersion.Parse("4.6.0"))
            {
                Wifi.ChangeWiFiInterfaceState(false, true);
                JObject obj = JObject.Parse(Wifi.GetWiFiInterfaceState());
                Assert.IsTrue(obj["powerSavingsState"].ToString() == "enabled");
                Wifi.ChangeWiFiInterfaceState(false, false);
                obj = JObject.Parse(Wifi.GetWiFiInterfaceState());
                Assert.IsTrue(obj["powerSavingsState"].ToString() == "disabled");
            }
            else
            {
                Assert.ThrowsException<DeviceError>(() => Wifi.ChangeWiFiInterfaceState(false, true));
            }
        }

        /// <summary>Tests setting wifi power savings via ChangeWiFiInterfaceState asserts from an incompatible device api version.<summary>
        [TestMethod]
        public void Check_ChangeWiFiInterfaceState_Asserts()
        {
            SinceDeviceAPIVersion.SetDeviceApiVersion("1.0.0");
            Assert.ThrowsException<DeviceError>(() => Wifi.ChangeWiFiInterfaceState(false, true));
        }

        /// <summary>Tests disabling wifi power savings via ChangeWiFiInterfaceState does not assert.<summary>
        [TestMethod]
        public void Check_ChangeWiFiInterfaceState_DoesNotAssert()
        {
            SinceDeviceAPIVersion.SetDeviceApiVersion("1.0.0");
            Wifi.ChangeWiFiInterfaceState(false, false);
        }
    }
}
