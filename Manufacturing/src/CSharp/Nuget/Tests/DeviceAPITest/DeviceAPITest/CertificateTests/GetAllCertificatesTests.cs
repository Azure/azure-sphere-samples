/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.CertificateTests
{
    /// <summary>
    /// A test class for the get all certificates api endpoint.
    /// </summary>
    [TestClass]
    public class GetAllCertificatesTests
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
        /// Tests if getting all certificates without adding a certificate returns an empty identifiers list.
        /// </summary>
        [TestMethod]
        public void AllCertificates_Get_ReturnsEmptyIdentifiersList()
        {
            string response = Certificate.GetAllCertificates();

            Assert.AreEqual("{\"identifiers\":[]}", response);
        }

        /// <summary>
        /// Tests if adding a certificate is reflected in calls to get all certificates.
        /// </summary>
        [TestMethod]
        public void
        AllCertificates_AddingIdentifierIncreasesList_ReturnsNonEmptyIdentifiersList()
        {
            string startResponse =
                Certificate.GetAllCertificates();

            Assert.AreEqual("{\"identifiers\":[]}", startResponse);

            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert,
                                       "rootca");

            string endResponse =
                Certificate.GetAllCertificates();

            Assert.AreEqual("{\"identifiers\":[\"TestCert\"]}", endResponse);
        }

        /// <summary>
        /// Tests if deleting a certificate is reflected in calls to get all certificates.
        /// </summary>
        [TestMethod]
        public void
        AllCertificates_DeletingIdentifierDecreasesList_ReturnsEmptyIdentifiersList()
        {
            Certificate.AddCertificate("TestCert", Utilities.pathToTestRootCert,
                                       "rootca");

            string response = Certificate.GetAllCertificates();

            Assert.AreEqual("{\"identifiers\":[\"TestCert\"]}", response);

            Certificate.RemoveCertificate("TestCert");

            string endResponse =
                Certificate.GetAllCertificates();

            Assert.AreEqual("{\"identifiers\":[]}", endResponse);
        }
    }
}
