/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json;

namespace TestDeviceRestAPI.ManufacturingTests
{
    /// <summary>
    /// A test class for the set manufacturing state api endpoint.
    /// </summary>
    [TestClass]
    public class SetManufacturingState
    {
        /// <summary>
        /// Tests if setting the manufacturing state with 'None' throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetManufacturingState_NullManufacturingState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Manufacturing.SetDeviceManufacturingState(null));
            Assert.AreEqual("Cannot set manufacturing state, manufacturing state supplied is invalid.", exception.Message);
        }

        /// <summary>
        /// Tests if setting the manufacturing state with an empty string throws a valiadation error.
        /// </summary>
        [TestMethod]
        public void SetManufacturingState_EmptyManufacturingState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Manufacturing.SetDeviceManufacturingState(""));
            Assert.AreEqual("Cannot set manufacturing state, manufacturing state supplied is invalid.", exception.Message);
        }

        /// <summary>
        /// Tests if setting the manufacturing state with an invalid value throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetManufacturingState_InvalidManufacturingState_ThrowsValidationError()
        {
            ValidationError exception = Assert.ThrowsException<ValidationError>(() => Manufacturing.SetDeviceManufacturingState("InvalidState"));
            Assert.AreEqual("Cannot set manufacturing state, manufacturing state supplied is invalid.", exception.Message);
        }

        /// <summary>
        /// Tests if setting the manufacturing state with an invalid value throws a validation error.
        /// </summary>
        [TestMethod]
        public void SetManufacturingState_SetManufacturingStateAsCurrent_ReturnsCurrentState()
        {
            string startGetResponse = Manufacturing.GetManufacturingState();
            string currentState = JsonConvert.DeserializeObject<Dictionary<string, string>>(startGetResponse)["manufacturingState"];

            string setResponse = Manufacturing.SetDeviceManufacturingState(currentState);

            Assert.AreEqual("{}", setResponse);

            string endGetResponse = Manufacturing.GetManufacturingState();

            Assert.AreEqual(endGetResponse, startGetResponse);

        }
    }
}
