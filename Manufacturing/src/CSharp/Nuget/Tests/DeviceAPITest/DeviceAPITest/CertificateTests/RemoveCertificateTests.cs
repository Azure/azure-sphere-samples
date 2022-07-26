/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.CertificateTests
{
    /// <summary>
    /// A test class for the remove certificate api endpoint.
    /// </summary>
    [TestClass]
    public class RemoveCertificateTests
    {
        /// <summary>
        /// Removes added certificates before each test.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanCertificates();
        }

        /// <summary>
        /// Tests if removing a certificate with a null certificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void RemoveCertificate_NullCertificateId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.RemoveCertificate(null));
            Assert.AreEqual(exception.Message, "Cannot remove certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if removing a certificate with an empty certificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void RemoveCertificate_EmptyCertificateId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.RemoveCertificate(""));
            Assert.AreEqual(exception.Message, "Cannot remove certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if removing an existing certificate returns an empty json response.
        /// </summary>
        [TestMethod]
        public void RemoveCertificate_RemovingExistingCertificate_ReturnsEmptyJsonResponse()
        {
            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert,
                                       "rootca");

            string response = Certificate.RemoveCertificate("TestCert");

            Assert.AreEqual("{}", response);
        }

        /// <summary>
        /// Tests if removing a non existing certificate throws a device error.
        /// </summary>
        [TestMethod]
        public void
        RemoveCertificate_RemovingNonExistingCertificate_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Certificate.RemoveCertificate("TestCert"));
            Assert.AreEqual(exception.Message, "This resource is unavailable on this device. No certificate with that identifier could be found on the device.");
        }

        /// <summary>
        /// Tests if removing a removed certificate throws a device error.
        /// </summary>
        [TestMethod]
        public void RemoveCertificate_RemovingRemovedCertificate_ThrowsDeviceError()
        {
            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert, "rootca");

            Certificate.RemoveCertificate("TestCert");

            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Certificate.RemoveCertificate("TestCert"));
            Assert.AreEqual(exception.Message, "This resource is unavailable on this device. No certificate with that identifier could be found on the device.");
        }
    }
}
