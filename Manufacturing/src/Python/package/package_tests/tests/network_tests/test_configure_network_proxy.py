# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, network


def test__configure_network_proxy__null_address_throws_validation_error():
    """Tests if giving null as the address throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, None, 0, None, "")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, address was null or empty.'


def test__configure_network_proxy__empty_address_throws_validation_error():
    """Tests if giving an empty string as the address throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "", 0, None, "")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, address was null or empty.'


def test__configure_network_proxy__null_authentication_type_throws_validation_error():
    """Tests if giving an empty string as the authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None, None)
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, invalid authentication type.'


def test__configure_network_proxy__empty_authentication_type_throws_validation_error():
    """Tests if giving an empty string as the authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None, "")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, invalid authentication type.'


def test__configure_network_proxy__invalid_authentication_type_throws_validation_error():
    """Tests if giving an invalid string as the authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty",
                                0, None, "NotNullOrEmpty")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, invalid authentication type.'


def test__configure_network_proxy__anonymous_with_username_and_password_throws_validation_error():
    """Tests if giving a username and password with an anonymous authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None,
                                "anonymous", "username", "password")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.'


def test__configure_network_proxy__anonymous_with_username_throws_validation_error():
    """Tests if giving a username with an anonymous authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty",
                                0, None, "anonymous", "username")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.'


def test__configure_network_proxy__anonymous_with_null_username_and_non_null_password_throws_validation_error():
    """Tests if giving a null username and a non null password with an anonymous authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None,
                                "anonymous", None, "password")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.'


def test__configure_network_proxy__basic_no_username_or_password_throws_validation_error():
    """Tests if giving no username or password with a basic authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None,
                                "basic")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.'


def test__configure_network_proxy__basic_None_username_and_password_throws_validation_error():
    """Tests if giving a username and a null password with a basic authentication type throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        network.configure_proxy(False, "NotNullOrEmpty", 0, None,
                                "basic", None, None)
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.'


def test__configure_network_proxy__anonymous_with_valid_values_returns_correct_values(fix_clean_proxies):
    """Tests if giving valid values with an anonymous authentication type succeeds."""
    response = network.configure_proxy(
        True, "example.com", 8081, [], "anonymous")
    assert response == {"address": "example.com", "enabled": True,
                        "port": 8081, "authenticationType": "anonymous", "noProxyAddresses": []}


def test__configure_network_proxy__basic_with_valid_values_returns_correct_values(fix_clean_proxies):
    """Tests if giving valid values with the basic authentication type succeeds."""
    response = network.configure_proxy(
        True, "example.com", 8081, [], "basic", "example", "example")
    assert response == {"address": "example.com", "enabled": True, "port": 8081,
                        "authenticationType": "basic", "username": "example", "password": "example", "noProxyAddresses": []}
