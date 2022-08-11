# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, wifi


def test__add_wifi_network__null_ssid_throws_validation_error(fix_setup_networks):
    """Tests if trying to add a wifi network with a null ssid throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network(None, "")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid SSID.'


def test__add_wifi_network__empty_ssid_throws_validation_error():
    """Tests if trying to add a network with an empty ssid throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("", "")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid SSID.'


def test__add_wifi_network__null_config_state_throws_validation_error():
    """Tests if trying to add a wifi network with a null config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("NotNullOrEmpty!", "", config_state=None)
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid config state.'


def test__add_wifi_network__empty_config_state_throws_validation_error():
    """Tests if trying to add a wifi network with an empty config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("NotNullOrEmpty", "", config_state="")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid config state.'


def test__add_wifi_network__invalid_config_state_throws_validation_error():
    """Tests if trying to add a wifi network with an invalid config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("NotNullOrEmpty", "",
                              config_state="InvalidState!")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid config state.'


def test__add_wifi_network__valid_config_state_returns_ok(fix_setup_networks):
    """Tests if trying to add a wifi network with valid config state returns the expected values list."""
    valid_states = ["enabled", "disabled"]

    for count in range(len(valid_states)):
        wifi.add_wifi_network(
            f"TestSsid{count}", "open", "example", valid_states[count])

    end_response = wifi.get_all_wifi_networks()

    assert {"values": [{"ssid": "TestSsid0", "configState": "enabled", "connectionState": "disconnected", "id": 0, "securityState": "open", "targetedScan": False, "configName": "example"}, {
        "ssid": "TestSsid1", "configState": "disabled", "connectionState": "disconnected", "id": 1, "securityState": "open", "targetedScan": False, "configName": "example"}]} == end_response


def test__add_wifi_network__null_security_state_throws_validation_error():
    """Tests if trying to add a wifi network with a null security state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("TestSsid", None, "example", "enabled")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid network type.'


def test__add_wifi_network__empty_security_state_throws_validation_error():
    """Tests if trying to add a wifi network with an empty security state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network("TestSsid", "", "example", "enabled")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid network type.'


def test__add_wifi_network__invalid_security_state_throws_validation_error():
    """Tests if trying to add a wifi network with an invalid security state throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        wifi.add_wifi_network(
            "TestSsid", "InvalidSecurityState!", "example", "enabled")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot add wifi network config, invalid network type.'


def test__add_wifi_network__valid_security_states_returns_expected_configurations(fix_setup_networks):
    """Tests if trying to add a wifi network with valid security states returns the expected network configurations."""

    eaptls = "eaptls"
    open = "open"
    psk = "psk"

    eaptls_response = wifi.add_wifi_network("TestEaptls",
                                            eaptls, "example1", "enabled", client_cert_store_identifier="example")
    assert eaptls_response == {"ssid": "TestEaptls", "configState": "enabled", "connectionState": "unknown", "id": 0,
                               "securityState": eaptls, "targetedScan": False, "configName": "example1", "clientCertStoreIdentifier": "example"}

    open_response = wifi.add_wifi_network("Testopen",
                                          open, "example2", "enabled")
    assert open_response == {"ssid": "Testopen", "configState": "enabled", "connectionState": "unknown", "id": 1,
                             "securityState": open, "targetedScan": False, "configName": "example2"}

    psk_response = wifi.add_wifi_network("Testpsk", psk,
                                         config_state="enabled", target_scan=False, psk="EXAMPLEPSK")
    assert psk_response == {"ssid": "Testpsk", "configState": "enabled", "connectionState": "unknown", "id": 2,
                            "securityState": psk, "targetedScan": False}
