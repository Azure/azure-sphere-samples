/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.CertificateTests
{
    /// <summary>
    /// A test class for the get certificate api endpoint.
    /// </summary>
    [TestClass]
    public class GetCertificateTests
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
        /// Getting a certificate with a null certificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetCertificate_NullCertificateId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.GetCertificate(null));
            Assert.AreEqual(exception.Message, "Cannot Remove Certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if getting a certificate with an empty certificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void GetCertificate_EmptyCertificateId_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.GetCertificate(""));
            Assert.AreEqual(exception.Message, "Cannot Remove Certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if getting an exisitng certificate returns the certificate.
        /// </summary>
        [TestMethod]
        public void GetCertificate_ValidCertificateId_ReturnsCertificate()
        {
            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert,
                                       "rootca");
            string response =
                Certificate.GetCertificate("TestCert");

            Assert.AreEqual(
                "{\"id\":\"TestCert\",\"notBefore\":\"2022-06-17T09:12:46\",\"notAfter\":\"2023-06-17T09:32:46\",\"subjectName\":\"\\/CN=TestRootCert\",\"issuerName\":\"\\/CN=TestRootCert\"}",
                response);
        }

        /// <summary>
        /// Tests if getting a certificate after it has been deleted throws a device error.
        /// </summary>
        [TestMethod]
        public void GetCertificate_DeleteCertificateThenGet_ThrowsDeviceError()
        {
            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert,
                                       "rootca");
            Certificate.RemoveCertificate("TestCert");

            DeviceError exception = Assert.ThrowsException<DeviceError>(() => Certificate.GetCertificate("TestCert"));
            Assert.AreEqual(exception.Message, "This resource is unavailable on this device. No certificate with that identifier could be found on the device.");

        }
    }
}
