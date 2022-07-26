/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the set network interfaces endpoint.
    /// </summary>
    [TestClass]
    public class SetNetworkInterfacesTests
    {
        /// <summary>Sets the wlan0 network interface to be true before each test.</summary>
        [TestCleanup]
        public void TestCleanup()
        {
            Network.SetNetworkInterfaces("wlan0", true);
        }

        /// <summary>Tests if setting an interface with a null name throws a validation error.</summary>
        [TestMethod]
        public void SetInterfaces_NullNetworkInterfaceName_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.SetNetworkInterfaces(null, false));
            Assert.AreEqual("Cannot set network interface, networkInterfaceName was null or empty!", exception.Message);
        }

        /// <summary>Tests if setting an interface with an empty name throws a validation error.</summary>
        [TestMethod]
        public void SetInterfaces_EmptyNetworkInterfaceName_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.SetNetworkInterfaces("", false));
            Assert.AreEqual("Cannot set network interface, networkInterfaceName was null or empty!", exception.Message);
        }

        /// <summary>Tests if setting the interface to false and true succeeds.</summary>
        [TestMethod]
        public void SetInterfaces_AlternateInterfaceUp_ChangeIsObserved()
        {
            //Ensure wlan0 is enabled
            Network.SetNetworkInterfaces("wlan0", true);

            //Disable wlan
            Network.SetNetworkInterfaces("wlan0", false);
            Assert.IsTrue(WaitForInterfaceChange(false));

            //Enable wlan
            Network.SetNetworkInterfaces("wlan0", true);
            Assert.IsTrue(WaitForInterfaceChange(true));
        }

        /// <summary>Test if setting interface up to false makes wifi endpoints stop responding.</summary>

        [TestMethod]
        public void SetInterfaces_SettingInterfaceUpToFalse_MakesWifiFail()
        {
            Network.SetNetworkInterfaces("wlan0", false);

            Assert.ThrowsException<DeviceError>(() => Wifi.GetWiFiScan());
            Assert.ThrowsException<DeviceError>(
                () => Wifi.GetAllConfiguredWifiNetworks());
            Assert.ThrowsException<DeviceError>(
                () => Wifi.DeleteWiFiNetConfig(0));
            Assert.ThrowsException<DeviceError>(
                () => Network.GetAllFailedNetworkConnections());

            Network.SetNetworkInterfaces("wlan0", true);
        }

        /// <summary>Helper function that waits for an interface state to change.<summary>
        private static bool WaitForInterfaceChange(bool interfaceState)
        {
            int maxTime = 5000;
            int timeElapsed = 0;
            while (timeElapsed < maxTime)
            {
                string content = Network.GetNetworkInterface("wlan0");
                WlanState state = JsonConvert.DeserializeObject<WlanState>(content);
                if (state.interfaceUp == interfaceState)
                {
                    return true;
                }
                else
                {
                    Thread.Sleep(250);
                    timeElapsed += 250;
                }
            }
            return false;
        }

        /// <summary>Class to convert a json version of a network interface into.<summary>
        public class WlanState
        {
            public string interfaceName { get; set; }
            public bool interfaceUp { get; set; }
            public bool connectedToNetwork { get; set; }
            public bool ipAcquired { get; set; }
            public bool connectedToInternet { get; set; }
            public string hardwareAddress { get; set; }
            public string ipAssignment { get; set; }
        }
    }
}
