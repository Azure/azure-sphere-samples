/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the get wifi interface state endpoint.
    /// </summary>
    [TestClass]
    public class GetWifiInterfaceStateTests
    {
        /// <summary>Tests if getting the wifi interface state returns an interface of the expected format.<summary>
        [TestMethod]
        public void GetWifiInterface_Get_ReturnsInterfaceExpectedFormat()
        {
            string stateSchema =
                @"{'type':'object', 'properties': {'configState':{'type':'string'}, 'connectionState':{'type':'string'}, 'securityState':{'type':'string'}, 'mode':{'type':'string'}, 'key_mgmt':{'type':'string'}, 'wpa_state':{'type':'string'}, 'address':{'type':'string'}, 'id':{'type':'integer'}}}";

            string response = Wifi.GetWiFiInterfaceState();

            Assert.IsTrue(
                JObject.Parse(response).IsValid(JSchema.Parse(stateSchema)));
        }
    }
}
