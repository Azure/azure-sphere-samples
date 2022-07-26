/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;
using TestDeviceRestAPI.Helpers;

namespace TestDeviceRestAPI.AppTests
{
    /// <summary>
    /// A test class for the set app status api endpoint.
    /// </summary>
    [TestClass]
    public class SetAppStatusTests
    {
        /// <summary>
        /// Removes all application images that are not gdb server.
        /// </summary>
        [TestInitialize]
        public void TestInitialize()
        {
            Utilities.CleanImages();
        }

        /// <summary>
        /// Tests if setting the app status with a null component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenComponentIdIsNull_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus(null, ""));
            Assert.AreEqual("Cannot set app status, invalid componentID!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with an empty component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenComponentIdIsEmpty_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus("", ""));
            Assert.AreEqual("Cannot set app status, invalid componentID!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with a non uuid format component id throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenComponentIdIsNotUuidFormat_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus("1234", ""));
            Assert.AreEqual("Cannot set app status, invalid componentID!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with a null trigger throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenTriggerIsNull_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus(Utilities.randomUUID, null));
            Assert.AreEqual("Cannot set app status, invalid state!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with an empty trigger throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenTriggerIsEmpty_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus(Utilities.randomUUID, ""));
            Assert.AreEqual("Cannot set app status, invalid state!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with a non-valid trigger throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenTriggerNotValid_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => App.SetAppStatus(Utilities.randomUUID, "InvalidString!"));
            Assert.AreEqual("Cannot set app status, invalid state!", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with a valid format component id that doesnt exist throws a device error.
        /// </summary>
        [TestMethod]
        public void
        SetAppStatus_WhenComponentIdIsUuidButDoesntExist_ThrowsDeviceError()
        {
            DeviceError exception = Assert.ThrowsException<DeviceError>(() => App.SetAppStatus(Utilities.randomUUID, "start"));
            Assert.AreEqual("ERROR: The device could not perform this request due to the resource being in a conflicting state. Application is not present", exception.Message);
        }

        /// <summary>
        /// Tests if setting the app status with valid triggers returns corresponding states.
        /// </summary>
        [TestMethod]
        public void SetAppStatus_WhenTriggerValid_ReturnsCorrectState()
        {
            Sideload.StageImage(Utilities.pathToBlinkImage);
            Sideload.InstallImages();

            string capaResponse = Capabilities.GetDeviceCapabilities();

            List<int> capabilities = JsonConvert.DeserializeObject<Dictionary<string, List<int>>>(capaResponse)["device_capabilities"];

            Assert.IsTrue(capabilities.Contains(11));

            Dictionary<string, string> validValuesAndStates =
                new(){{"start", "{\"state\":\"running\"}"},
                 {"stop", "{\"state\":\"stopped\"}"}};

            foreach (string triggerValue in validValuesAndStates.Keys)
            {
                string response = App.SetAppStatus(Utilities.blinkComponentId, triggerValue);

                Assert.AreEqual(validValuesAndStates[triggerValue], response);
            }

            // Check gdbserver running
            List<Dictionary<string, object>> images = Utilities.GetImageComponents();

            // Remove all images that are not applications
            foreach (Dictionary<string, object> img in images)
            {
                Console.WriteLine(img["name"]);
            }

            int count =
                images.RemoveAll(elem => ((string)elem["name"]).Equals("gdbserver"));

            string debugResponse = App.SetAppStatus(Utilities.blinkComponentId, "startDebug");

            if (count == 0)
            {
                Assert.AreEqual(
                    "{\"error\":\"Cannot set app status, request failed!\"}",
                    debugResponse);
            }
            else
            {
                Assert.AreEqual(
                    "{\"state\":\"debugging\",\"gdbPort\":2345,\"outPort\":2342}",
                    debugResponse);
            }
        }
    }
}
