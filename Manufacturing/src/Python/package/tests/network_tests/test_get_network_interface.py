# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from microsoft_azure_sphere_deviceapi import exceptions, network


def test__get_network_interface__null_interface_throws_validation_error():
    """Tests if getting a null interface throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.get_network_interface(None)
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot get network interface, input was null or empty.'


def test__get_network_interface__empty_interface_throws_validation_error():
    """Tests if gettting an empty interface throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.get_network_interface("")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot get network interface, input was null or empty.'


def test__get_network_interface__invalid_interface_throws_validation_error():
    """Tests if getting a non existent interface throws a device error."""
    with pytest.raises(exceptions.DeviceError) as error:
        network.get_network_interface("InvalidInterface!")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.DeviceError: ERROR: This resource is unavailable on this device. Invalid request URI.'


def test__get_network_interface__valid_interface_returns_expected_interface():
    """Tests if getting a valid interface returns the expected values."""
    response = network.get_network_interface("azspheresvc")

    assert {"interfaceName": "azspheresvc", "interfaceUp": True, "connectedToNetwork": False,
            "ipAcquired": False, "connectedToInternet": False, "ipAddresses": ["192.168.35.2"]} == response
