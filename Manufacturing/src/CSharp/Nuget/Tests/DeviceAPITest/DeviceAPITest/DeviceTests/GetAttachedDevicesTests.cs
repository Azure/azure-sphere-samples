/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get attached devices api endpoint.
    /// </summary>
    [TestClass]
    public class GetAttachedDevicesTests
    {
        /// <summary>
        /// Getting the attached devices returns a list of items in the correct format.
        /// </summary>
        [TestMethod]
        public void AttachedDevices_Get_ReturnsCorrectFormatDevices()
        {
            string deviceSchema = @"{'type':'object', 'properties': {'DeviceConnectionPath':{'type':'string'}, 'IpAddress':{'type':'string'}}}";

            string response = Devices.GetAttachedDevices();
            JArray parsedResponse = JsonConvert.DeserializeObject<JArray>(response);
            JSchema parsedDeviceSchema = JSchema.Parse(deviceSchema);

            foreach (JObject device in parsedResponse)
            {
                Assert.IsTrue(device.IsValid(parsedDeviceSchema));
            }
        }
    }
}
