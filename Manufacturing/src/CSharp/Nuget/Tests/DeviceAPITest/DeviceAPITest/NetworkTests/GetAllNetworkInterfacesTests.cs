/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get all network interfaces endpoint.
    /// </summary>
    [TestClass]
    public class GetAllNetworkInterfacesTests
    {
        /// <summary>Tests if getting all network interfaces, returns interfaces in the correct format.</summary>
        [TestMethod]
        public void GetNetworkInterfaces_Get_ReturnsInterfacesInCorrectFormat()
        {
            string interfacesSchema = @"{'type': 'object','properties': {'interfaceName': {'type':'string'}, 'interfaceUp': {'type':'boolean'},'connectedToNetwork': {'type':'boolean'},'ipAcquired': {'type':'boolean'},'connectedToInternet': {'type':'boolean'},'ipAddresses': {'type':'array'}, 'hardwareAddress': {'type':'string'}, 'ipAssignment': {'type':'string'}}, 'oneOf': [{'required': ['interfaceName', 'interfaceUp', 'connectedToNetwork', 'ipAcquired', 'connectedToInternet', 'ipAddresses']}, {'required': ['interfaceName', 'interfaceUp', 'connectedToNetwork', 'ipAcquired', 'connectedToInternet', 'hardwareAddress', 'ipAssignment']}]}";

            JSchema parsedInterSchema = JSchema.Parse(interfacesSchema);

            Network.SetNetworkInterfaces("wlan0", true);
            string response =
                Network.GetAllNetworkInterfaces();

            foreach (JObject inter in JsonConvert.DeserializeObject<JObject>(response)["interfaces"])
            {
                Assert.IsTrue(inter.IsValid(parsedInterSchema));
            }
        }
    }
}
