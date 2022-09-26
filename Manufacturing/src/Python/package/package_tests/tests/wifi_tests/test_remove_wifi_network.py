# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, wifi


def test__remove_network__no_network_returns_not_found(fix_setup_networks):
    """Tests if removing a network that doesnt exist throws a device error."""
    with pytest.raises(exceptions.DeviceError, match="Network not found") as error:
        wifi.remove_configured_wifi_network(0)


def test__remove_network__add_then_remove_returns_ok(fix_setup_networks):
    """Tests if removing an added network removes the network"""
    wifi.add_wifi_network("Test", "open", "", "enabled")

    start_response = wifi.get_all_wifi_networks()
    assert start_response == {'values': [{'ssid': 'Test', 'configState': 'enabled',
                                          'connectionState': 'disconnected', 'id': 0, 'securityState': 'open', 'targetedScan': False}]}

    response = wifi.remove_configured_wifi_network(0)

    assert response == {}

    end_response = wifi.get_all_wifi_networks()

    assert end_response == {'values': []}


def test__remove_network__add_then_remove_returns_non_empty_list(fix_setup_networks):
    """Tests if removing an added network returns an empty json response."""
    wifi.add_wifi_network("Test", "open", "", "enabled")

    response = wifi.remove_configured_wifi_network(0)

    assert response == {}

    end_response = wifi.get_all_wifi_networks()

    assert end_response == {'values': []}
