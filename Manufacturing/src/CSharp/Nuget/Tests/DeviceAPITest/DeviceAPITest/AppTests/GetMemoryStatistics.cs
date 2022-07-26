/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.AppTests
{
    /// <summary>
    /// A test class for the get memory statistics api endpoint.
    /// </summary>
    [TestClass]
    public class GetMemoryStatistics
    {
        /// <summary>
        /// Tests if getting the memory statistics returns in the expected format.
        /// </summary>
        [TestMethod]
        public void GetMemoryStatistics_ReturnsValidResponseFormat()
        {
            string response = App.GetMemoryStatistics();

            Assert.IsTrue(IsValidMemoryStatisticsFormat(response));
        }

        /// <summary>
        /// Helper function to validate the format of a response matches the expected format.
        /// </summary>
        /// <param name="response">The response to validate.</param>
        /// <returns>True if the response is successfully validted against the schema, false otherwise.</returns>
        private static bool IsValidMemoryStatisticsFormat(string response)
        {
            JObject keyValuePairs = JsonConvert.DeserializeObject<JObject>(response);
            Assert.IsTrue(keyValuePairs.ContainsKey("memoryStats"));
            string expectedResponseSchema =
                @"{'type':'object', 'properties': {'currentMemoryUsageInBytes':{'type': 'integer'},'userModeMemoryUsageInByte':{'type': 'integer'},'peakUserModeMemoryUsageInBytes':{'type': 'integer'}}}";
            JSchema parsedExpectedResponseSchema = JSchema.Parse(expectedResponseSchema);
            return keyValuePairs["memoryStats"].IsValid(parsedExpectedResponseSchema);
        }
    }
}
