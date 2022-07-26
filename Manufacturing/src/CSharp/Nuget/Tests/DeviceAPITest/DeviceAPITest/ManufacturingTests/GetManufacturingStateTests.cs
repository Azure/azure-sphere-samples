/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.ManufacturingTests
{
    /// <summary>
    /// A test class for the get manufacturing state api endpoint.
    /// </summary>
    [TestClass]
    public class GetManufacturingStateTests
    {
        /// <summary>
        /// Tests if get manufacturing returns a response of the correct format.
        /// </summary>
        [TestMethod]
        public void GetManufacturingState_Get_ReturnsValidManufacturingState()
        {
            string response = Manufacturing.GetManufacturingState();

            string expectedTopLevelSchema =
                @"{'type': 'object','properties': {'manufacturingState': {'type':'string', 'required':true}}}";

            string[] validManufacturingStates = { "Blank", "Module1Complete", "DeviceComplete", "Unknown" };

            Assert.IsTrue(JObject.Parse(response).IsValid(JSchema.Parse(expectedTopLevelSchema)));

            string state = JsonConvert.DeserializeObject<Dictionary<string, string>>(response)["manufacturingState"];

            Assert.IsTrue(validManufacturingStates.Any(validState => validState.Equals(state)));
        }
    }
}
