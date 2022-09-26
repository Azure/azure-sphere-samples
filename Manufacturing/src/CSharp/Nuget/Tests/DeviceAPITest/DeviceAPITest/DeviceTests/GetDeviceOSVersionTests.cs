/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;
using System.Management.Automation;


namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get device OS version api endpoint.
    /// </summary>
    [TestClass]
    public class GetDeviceOSVersionTests
    {
        /// <summary>
        /// Tests if getting the device OS version returns the expected format.
        /// </summary>
        [TestMethod]
        public void GetDeviceOSVersion_Get_ReturnsExpectedVersion()
        {
            string restApiVersion = Device.GetDeviceRestAPIVersion();
            JObject resp = JObject.Parse(restApiVersion);
            Console.WriteLine(restApiVersion);
            Console.WriteLine(resp);
            Console.WriteLine(resp["REST-API-Version"]);

            SemanticVersion restSemVer = SemanticVersion.Parse(resp["REST-API-Version"].ToString());
            Console.WriteLine(restSemVer);
            SemanticVersion validVersion = SemanticVersion.Parse("4.5.0");
            Console.WriteLine(validVersion);
            Console.WriteLine(restSemVer.CompareTo(validVersion));

            if (restSemVer.CompareTo(validVersion) >= 0)
            {
                string osVersionSchema = @"{'type': 'object','properties': {'osversion':{'type': 'string'}}}";
                string osVersionResp = Device.GetDeviceOSVersion();
                JObject osVersionJson = JObject.Parse(osVersionResp);

                Assert.IsTrue(osVersionJson.IsValid(JSchema.Parse(osVersionSchema)));
                SemanticVersion.Parse(osVersionJson["osversion"].ToString());
            }
            else
            {
                Assert.ThrowsException<AzureSphereException>(() => Device.GetDeviceOSVersion());
            }
        }
    }
}
