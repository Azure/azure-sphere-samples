# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, wifi


def test__change_wifi_network__null_config_state_returns_bad_request():
    """Tests if changing a configured network with a null config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot change Wi-Fi network config, config state is invalid"):
        wifi.change_wifi_network_config(0, None, "")


def test__change_wifi_network__empty_config_state_returns_bad_request():
    """Tests if changing a configured network with an empty config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot change Wi-Fi network config, config state is invalid"):
        wifi.change_wifi_network_config(0, "", "")


def test__change_wifi_network__invalid_config_state_returns_bad_request():
    """Tests if changing a configured network with an invalid config state throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot change Wi-Fi network config, config state is invalid"):
        wifi.change_wifi_network_config(0, "InvalidConfigState!", "")


def test__change_wifi_network__null_psk_returns_bad_request():
    """Tests if changing a configured network with a null psk state throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot change Wi-Fi network config, psk is null or empty"):
        wifi.change_wifi_network_config(0, "unknown", None)


def test__change_wifi_network__empty_psk_returns_bad_request():
    """Tests if changing a configured network with an empty psk throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot change Wi-Fi network config, psk is null or empty"):
        wifi.change_wifi_network_config(0, "unknown", "")


def test__change_wifi_network__change_non_existent_network_throws_device_error(fix_setup_networks):
    """Tests if changing a non existent network throws a device error."""
    with pytest.raises(exceptions.DeviceError, match="ERROR: An invalid request was made to the device. PSK is too short"):
        wifi.change_wifi_network_config(0, "unknown", "Test")


def test__change_wifi_network__change_existing_network_changes_network_configuration(fix_setup_networks):
    """Tests if changing an existing network returns the expected network configuration."""
    wifi.add_wifi_network("NETWORK1", "psk", config_state="enabled",
                          target_scan=False, psk="EXAMPLEPSK")

    response = wifi.change_wifi_network_config(0, "disabled", "NETWORK1")

    assert response == {'configState': 'disabled', 'connectionState': 'disconnected',
                        'id': 0, 'securityState': 'psk', 'ssid': 'NETWORK1', 'targetedScan': False}
