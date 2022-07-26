/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.ACapabilitiesTest
{
    /// <summary>
    /// A test class for the get device capabilities api endpoint.
    /// </summary>
    [TestClass]
    public class GetDeviceCapabilitiesTest
    {
        /// <summary>
        /// Tests if getting the device capabilities shows that the expected capabilities are present and are in the correct format.
        /// </summary>
        [TestMethod]
        public void GetDeviceCapabilities_Get_ReturnsExpectedFormatCapabilities()
        {
            string responseSchema =
                @"{'type': 'object','properties': {'device_capabilities': {'type':'array'}}}";

            string response = Capabilities.GetDeviceCapabilities();

            Assert.IsTrue(JObject.Parse(response).IsValid(JSchema.Parse(responseSchema)));

            // Will throw exception and fail test if cannot deserialize into list of ints
            JsonConvert.DeserializeObject<Dictionary<string, List<int>>>(response);
        }
    }
}
