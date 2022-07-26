/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get device status api endpoint.
    /// </summary>
    [TestClass]
    public class GetDeviceStatusTests
    {
        /// <summary>
        /// Tests if getting the device status returns a response in the correct format.
        /// </summary>
        [TestMethod]
        public void GetDeviceStatus_Get_ReturnsCorrectFormat()
        {
            string statusSchema = @"{'type': 'object','properties': {'uptime': {'type':'integer'}}}";

            string response = Device.GetDeviceStatus();

            Assert.IsTrue(JObject.Parse(response).IsValid(JSchema.Parse(statusSchema)));
        }

        /// <summary>
        /// Tests if getting the device status at later points increases the 'uptime'.
        /// </summary>
        [TestMethod]
        public void GetDeviceStatus_GetThenGet_UptimeIncreasesWithLaterCalls()
        {
            string response = Device.GetDeviceStatus();
            int startTime = JsonConvert.DeserializeObject<Dictionary<string, int>>(response)["uptime"];

            Thread.Sleep(1000);

            string endResponse = Device.GetDeviceStatus();

            int endTime = JsonConvert.DeserializeObject<Dictionary<string, int>>(endResponse)["uptime"];

            Assert.IsTrue(startTime < endTime);
        }
    }
}
