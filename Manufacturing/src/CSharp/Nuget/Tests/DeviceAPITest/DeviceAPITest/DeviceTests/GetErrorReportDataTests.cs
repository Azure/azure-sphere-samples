/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the get error report data api endpoint.
    /// </summary>
    [TestClass]
    public class GetErrorReportDataTests
    {
        /// <summary>
        /// Tests if getting the error report data returns a non null or empty response.
        /// </summary>
        [TestMethod]
        public void GetErrorReportData_Get_ReturnsNonNullOrEmpty()
        {
            string response = Device.GetErrorReportData();

            Assert.IsFalse(string.IsNullOrEmpty(response));
        }
    }
}
