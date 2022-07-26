/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get diagnostics log api endpoint.
    /// </summary>
    [TestClass]
    public class GetDiagnosticsLogTests
    {
        /// <summary>
        /// Tests if getting the diagnostics log returns a non null or empty response.
        /// </summary>
        [TestMethod]
        public void GetDiagnosticsLog_Get_ReturnsNonNullOrEmptyResponse()
        {
            string response = Device.GetDiagnosticLog();

            Assert.IsFalse(string.IsNullOrEmpty(response));
        }
    }
}
