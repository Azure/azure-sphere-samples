/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;

namespace TestDeviceRestAPI.NetworkTests
{

    /// <summary>
    /// A test class for the configure proxy api endpoint.
    /// </summary>
    [TestClass]
    public class ConfigureProxyTests
    {

        /// <summary>Deletes any added network proxies before each test.</summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Network.DeleteNetworkProxy();
        }

        /// <summary>Tests if giving null as the address throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_NullAddress_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, null, 0, null, ""));
            Assert.AreEqual("Cannot configure proxy, address was null or empty!", exception.Message);
        }

        /// <summary>Tests if giving an empty string as the address throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_EmptyAddress_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, "", 0, null, ""));
            Assert.AreEqual("Cannot configure proxy, address was null or empty!", exception.Message);
        }

        /// <summary>Tests if giving an empty string as the address throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_NullAuthenticationType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, "NotNullOrEmpty", 0, null, null));
            Assert.AreEqual("Cannot configure proxy, authenticationType is invalid!", exception.Message);
        }

        /// <summary>Tests if giving an empty string as the authentication type throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_EmptyAuthenticationType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, "NotNullOrEmpty", 0, null, ""));
            Assert.AreEqual("Cannot configure proxy, authenticationType is invalid!", exception.Message);
        }

        /// <summary>Tests if giving an invalid string as the authentication type throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_InvalidAuthenticationType_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(
                false, "NotNullOrEmpty", 0, null, "NotNullOrEmpty"));
            Assert.AreEqual("Cannot configure proxy, authenticationType is invalid!", exception.Message);
        }

        /// <summary>Tests if giving a username and password with an anonymous authentication type throws a validation error.</summary>
        [TestMethod]
        public void
        ConfigureProxy_AnonymousWithUsernameAndPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, "NotNullOrEmpty", 0, null, "anonymous",
                                       "username", "password"));
            Assert.AreEqual("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!", exception.Message);
        }

        /// <summary>Tests if giving a username with an anonymous authentication type throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_AnonymousWithUsername_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(
                false, "NotNullOrEmpty", 0, null, "anonymous", "username"));
            Assert.AreEqual("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!", exception.Message);
        }

        /// <summary>Tests if giving a null username and a non null password with an anonymous authentication type throws a validation error.</summary>
        [TestMethod]
        public void
        ConfigureProxy_AnonymousWithNullUsernameAndNonNullPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(
                false, "NotNullOrEmpty", 0, null, "anonymous", null, "password"));
            Assert.AreEqual("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!", exception.Message);
        }

        /// <summary>Tests if giving no username or password with a basic authentication type throws a validation error.</summary>
        [TestMethod]
        public void ConfigureProxy_BasicNoUsernameOrPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(false, "NotNullOrEmpty", 0, null, "basic"));
            Assert.AreEqual("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!", exception.Message);
        }

        /// <summary>Tests if giving a username and a null password with a basic authentication type throws a validation error.</summary>
        [TestMethod]
        public void
        ConfigureProxy_BasicNullUsernameAndPassword_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Network.ConfigureProxy(
                false, "NotNullOrEmpty", 0, null, "basic", null, null));
            Assert.AreEqual("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!", exception.Message);
        }

        /// <summary>Tests if giving valid values with an anonymous authentication type succeeds.</summary>
        [TestMethod]
        public void ConfigureProxy_AnonymousWithValidValues_ReturnsCorrectProxyConfiguration()
        {
            string response = Network.ConfigureProxy(
                true, "example.com", 8081, new List<string>(), "anonymous");

            Assert.AreEqual(
                "{\"address\":\"example.com\",\"enabled\":true,\"port\":8081,\"authenticationType\":\"anonymous\",\"noProxyAddresses\":[]}",
                response);
        }

        /// <summary>Tests if giving valid values with the basic authentication type succeeds.</summary>
        [TestMethod]
        public void ConfigureProxy_BasicWithValidValues_ReturnsCorrectProxyConfiguration()
        {
            string response =
                Network.ConfigureProxy(true, "example.com", 8081, new List<string>(),
                                       "basic", "example", "example");

            Assert.AreEqual(
                "{\"address\":\"example.com\",\"enabled\":true,\"port\":8081,\"authenticationType\":\"basic\",\"username\":\"example\",\"password\":\"example\",\"noProxyAddresses\":[]}",
                response);
        }
    }
}
