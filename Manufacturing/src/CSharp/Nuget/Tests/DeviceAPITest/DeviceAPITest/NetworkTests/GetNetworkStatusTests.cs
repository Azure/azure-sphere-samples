/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get network status endpoint.
    /// </summary>
    [TestClass]
    public class GetNetworkStatusTests
    {
        /// <summary>Tests if getting the network status returns the expected values.</summary>
        [TestMethod]
        public void NetworkStatus_Get_ReturnsExpectedStatus()
        {
            string response = Network.GetNetworkStatus();

            string schemaJson = @"{
                'type': 'object',
                'properties': 
                {
                    'deviceAuthenticationIsReady': {'type': 'boolean'},
                    'networkTimeSync': {'type': 'string'},
                    'proxy': {'type': 'string'},
                }
            }";

            JsonSchema schema = JsonSchema.Parse(schemaJson);
            JObject networkStatus = JObject.Parse(response);
            bool valid = networkStatus.IsValid(schema);

            Assert.IsTrue(valid);
        }
    }
}
