/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.WifiTests
{
    /// <summary>
    /// A test class for the get wifi scan endpoint.
    /// </summary>
    [TestClass]
    public class GetWifiScanResultsTests
    {

        /// <summary>Tests if get wifi scan returns wifi networks of the expected format.<summary>
        [TestMethod]
        public void GetWifiScan_Get_ReturnsValidFormat()
        {
            string valuesSchema =
                @"{'type':'object', 'properties': {'values':{'type':'array'}}}";
            string wifiSchema =
                @"{'type':'object', 'properties': {'bssid':{'type':'string'}, 'freq':{'type':'integer'}, 'signal_level':{'type':'integer'}, 'ssid':{'type':'string'}, 'securityState':{'type':'string'}}}";
            string response = Wifi.GetWiFiScan();


            JObject values = JObject.Parse(response);

            Assert.IsTrue(values.IsValid(JSchema.Parse(valuesSchema)));

            foreach (JObject wifis in values["values"])
            {
                Assert.IsTrue(wifis.IsValid(JSchema.Parse(wifiSchema)));
            }
        }
    }
}
