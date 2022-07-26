/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the restart devices api endpoint.
    /// </summary>
    [TestClass]
    public class RestartDeviceTests
    {
        /// <summary>
        /// Tests if restarting the device returns that it is restarting the system.
        /// </summary>
        [TestMethod]
        public void RestartDevice_Call_ReturnsRestartingSystem()
        {
            Assert.IsTrue(WaitTillUp());
            string response = Device.RestartDevice();

            Assert.AreEqual("{\"restartingSystem\":true}", response);
        }

        /// <summary>
        /// Tests if restarting the device restarts the uptime.
        /// </summary>
        [TestMethod]
        public void RestartDevice_Call_RestartsUptime()
        {
            Assert.IsTrue(WaitTillUp());

            Thread.Sleep(5000);
            string startResponse = Device.GetDeviceStatus();
            int startUptime = JsonConvert.DeserializeObject<Dictionary<string, int>>(startResponse)["uptime"];

            Device.RestartDevice();

            Assert.IsTrue(WaitTillUp());

            string endResponse = Device.GetDeviceStatus();
            int endUptime = JsonConvert.DeserializeObject<Dictionary<string, int>>(endResponse)["uptime"];

            Assert.IsTrue(endUptime < startUptime);
        }

        /// <summary>
        /// Helper function that waits till the device has restarted or reaches a timeout then returns.
        /// </summary>
        /// <returns>True if device has responded in time, false otherwise.</returns>
        private static bool WaitTillUp()
        {
            int maxWaitTime = 5000;
            int currentWaitTime = 0;
            while (currentWaitTime < maxWaitTime)
            {
                try
                {
                    Device.GetDeviceStatus();
                    return true; ;
                }
                catch (HttpRequestException)
                {
                    Thread.Sleep(250);
                    currentWaitTime += 250;
                }
            }
            return false;
        }
    }
}
