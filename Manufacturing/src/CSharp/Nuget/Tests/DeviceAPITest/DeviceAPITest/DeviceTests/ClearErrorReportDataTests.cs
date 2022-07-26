/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using System.Text;


namespace TestDeviceRestAPI.DeviceTests
{
    /// <summary>
    /// A test class for the clear error report data api endpoint.
    /// </summary>
    [TestClass]
    public class ClearErrorReportDataTests
    {
        /// <summary>
        /// Tests if clearing the error report data returns an empty json response.
        /// </summary>
        [TestMethod]
        public void ClearErrorReportData_Call_ReturnsEmptyJsonResponse()
        {
            string response = Device.ClearErrorReportData();

            Assert.AreEqual(response, "{}");
        }

        /// <summary>
        /// Tests if clearing the error report data clears the error report data.
        /// </summary>
        [TestMethod]
        public void ClearErrorReportData_Call_ClearsErrorReportData()
        {
            string response = Device.ClearErrorReportData();

            Assert.AreEqual(response, "{}");

            //Wait for error data to clear or timeout 
            int maxMilliseconds = 1000;
            int elapsedMilliseconds = 0;

            int dataLength = GetDataLength(Device.GetErrorReportData());
            while (dataLength != 0 && elapsedMilliseconds < maxMilliseconds)
            {
                Thread.Sleep(100);
                elapsedMilliseconds += 100;
                dataLength = GetDataLength(Device.GetErrorReportData());

            }

            Assert.AreEqual(dataLength, 0);
        }

        /// <summary>
        /// Helper class that gets the data length bytes of the error reponse data.
        /// </summary>
        /// <param name="response">The error reponse data.</param>
        /// <returns>The data length bytes from the error response data.</returns>
        private static short GetDataLength(string response)
        {
            byte[] parsedResponse = Encoding.ASCII.GetBytes(response);

            return (short)(parsedResponse[3] << 8 | parsedResponse[4]);
        }
    }
}
