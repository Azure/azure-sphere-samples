/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.CertificateTests
{
    /// <summary>
    /// A test class for the add certificate api endpoint.
    /// </summary>
    [TestClass]
    public class AddCertificateTests
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
        /// Tests if adding a certificate with a null certificate location throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_NullCertificateLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("", null, "Ignore", "Ignore", "Ignore"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate location is null or empty.");
        }

        /// <summary>
        /// Tests if adding a certificate with an empty string as the certificate location throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_EmptyCertificateLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("", "", "Ignore", "Ignore", "Ignore"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate location is null or empty.");
        }

        /// <summary>
        /// Tests if adding a certificate with an invalid file location throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_InvalidFileLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ExampleID", "InvalidCertificateLocation", "Ignore"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, file doesn't exist or cannot open.");
        }

        /// <summary>
        /// Tests if adding a certificate with a null cerrtificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_NullCertificateID_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate(null, Utilities.pathToTestRootCert, "Ignore"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if adding a certificate with an empty certificate id throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_EmptyCertificateID_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("", Utilities.pathToTestRootCert, "Ignore"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate ID is null or empty.");
        }

        /// <summary>
        /// Tests if adding a certificate with a null certificate type throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_NullCertType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ExampleID", Utilities.pathToTestRootCert, null));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate type is invalid.");
        }

        /// <summary>
        /// Tests if adding a certificate with an empty certificate type throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_EmptyCertType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ExampleID", Utilities.pathToTestRootCert, ""));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate type is invalid.");
        }

        /// <summary>
        /// Tests if adding a certificate with an invalid certificate type throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_InvalidCertType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ExampleID", Utilities.pathToTestRootCert, "InvalidCertType!"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, certificate type is invalid.");
        }

        /// <summary>
        /// Tests if adding a certificate with valid certificate types returns empty json responses.
        /// </summary>
        [TestMethod]
        public void AddCertificate_ValidCertTypes_ReturnsEmptyJsonResponse()
        {
            // rootca
            string rootResponse =
                Certificate.AddCertificate("RootID", Utilities.pathToTestRootCert,
                                           "rootca");

            Assert.AreEqual("{}", rootResponse);

            // client
            string clientResponse =
                Certificate.AddCertificate(
                    "ClientID", Utilities.pathToTestClientCert, "client",
                    Utilities.pathToTestClientPrivateKey, "password");

            Assert.AreEqual("{}", clientResponse);
        }

        /// <summary>
        /// Tests if adding a client certificate with a null private key location throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_ClientNullPrivateKeyLocation_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ClientID", Utilities.pathToTestClientCert, "client", null, "password"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, private key location is null or empty.");
        }

        /// <summary>
        /// Tests if adding a client certificate with an empty string as a private key location throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_ClientEmptyPrivateKey_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ClientID", Utilities.pathToTestClientCert, "client", "", "password"));
            Assert.AreEqual(exception.Message, "Cannot add certificate, private key location is null or empty.");
        }

        /// <summary>
        /// Tests if adding a client certificate with a null password throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_ClientNullPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ClientID", Utilities.pathToTestClientCert, "client", Utilities.pathToTestClientPrivateKey, null));
            Assert.AreEqual(exception.Message, "Cannot add certificate, password is null or empty.");
        }

        /// <summary>
        /// Tests if adding a client certificate with an empty password throws a validation error.
        /// </summary>
        [TestMethod]
        public void AddCertificate_ClientEmptyPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Certificate.AddCertificate("ClientID", Utilities.pathToTestClientCert, "client", Utilities.pathToTestClientPrivateKey, ""));
            Assert.AreEqual(exception.Message, "Cannot add certificate, password is null or empty.");
        }
    }
}
