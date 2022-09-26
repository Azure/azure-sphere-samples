/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;
using System.Management.Automation;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get device rest api version api endpoint.
    /// </summary>
    [TestClass]
    public class GetDeviceRestAPIVersionTests
    {
        /// <summary>
        /// Tests if getting the device rest api version returns the expected version in the expected format.
        /// </summary>
        [TestMethod]
        public void GetDeviceRestAPIVersion_Get_ReturnsExpectedVersion()
        {
            string statusSchema = @"{'type': 'object','properties': {'REST-API-Version': {'type':'string'}}}";

            string response = Device.GetDeviceRestAPIVersion();
            JObject jsonResponse = JObject.Parse(response);
            Assert.IsTrue(jsonResponse.IsValid(JSchema.Parse(statusSchema)));
            SemanticVersion.Parse(jsonResponse["REST-API-Version"].ToString());
        }
    }
}
