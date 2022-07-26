/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get network interface endpoint.
    /// </summary>
    [TestClass]
    public class GetNetworkInterfaceTests
    {
        /// <summary>Tests if getting a null interface throws a validation error.</summary>
        [TestMethod]
        public void GetNetworkInterface_NullInterface_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.GetNetworkInterface(null));
            Assert.AreEqual("Cannot get network interface, input was null or empty!", exception.Message);
        }

        /// <summary>Tests if gettting an empty interface throws a validation error.</summary>
        [TestMethod]
        public void GetNetworkInterface_EmptyInterface_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.GetNetworkInterface(""));
            Assert.AreEqual("Cannot get network interface, input was null or empty!", exception.Message);
        }

        /// <summary>Tests if getting a valid interface returns the expected values.</summary>
        [TestMethod]
        public void GetNetworkInterface_ValidInterface_ReturnsExpectedValues()
        {
            string response =
                Network.GetNetworkInterface("azspheresvc");

            Assert.AreEqual(
                "{\"interfaceName\":\"azspheresvc\",\"interfaceUp\":true,\"connectedToNetwork\":false,\"ipAcquired\":false,\"connectedToInternet\":false,\"ipAddresses\":[\"192.168.35.2\"]}",
                response);
        }

        /// <summary>Tests if getting a non existent interface throws a device error.</summary>
        [TestMethod]
        public void GetNetworkInterface_InvalidInterface_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Network.GetNetworkInterface("InvalidInterface!"));
            Assert.AreEqual("This resource is unavailable on this device. Invalid request URI.", exception.Message);
        }
    }
}
