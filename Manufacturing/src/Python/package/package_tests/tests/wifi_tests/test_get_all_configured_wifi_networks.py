# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import wifi


def test__get_configured_wifi_networks__initially_returns_empty(fix_setup_networks):
    """Tests if getting all wifi networks before adding any returns an empty values list."""
    response = wifi.get_all_wifi_networks()

    assert response == {'values': []}


def test__get_configured_wifi_networks__add_item_returns_non_empty(fix_setup_networks):
    """Tests if getting all wifi networks after adding one shows the added network in the values list."""
    wifi.add_wifi_network("Test", "open", "", "enabled")

    response = wifi.get_all_wifi_networks()

    assert response == {'values': [{'ssid': 'Test', 'configState': 'enabled',
                                    'connectionState': 'disconnected', 'id': 0, 'securityState': 'open', 'targetedScan': False}]}


def test__get_configured_wifi_networks__add_multiple_items_returns_multiple_in_list(fix_setup_networks):
    """Tests if adding multiple wifi networks then calling get all wifi networks shows all the wifi networks."""
    wifi.add_wifi_network("Test1", "open", "", "enabled")
    wifi.add_wifi_network("Test2", "open", "", "enabled")
    response = wifi.get_all_wifi_networks()

    assert response == {'values': [{'ssid': 'Test1', 'configState': 'enabled', 'connectionState': 'disconnected', 'id': 0, 'securityState': 'open', 'targetedScan': False}, {
        'ssid': 'Test2', 'configState': 'enabled', 'connectionState': 'disconnected', 'id': 1, 'securityState': 'open', 'targetedScan': False}]}


def test__get_configured_wifi_networks__add_then_delete_returns_empty_list(fix_setup_networks):
    """Tests if adding then deleting a wifi network shows the correct state at each stage."""
    wifi.add_wifi_network("Test1", "open", "", "enabled")
    response = wifi.get_all_wifi_networks()
    assert response == {'values': [{'ssid': 'Test1', 'configState': 'enabled',
                                    'connectionState': 'disconnected', 'id': 0, 'securityState': 'open', 'targetedScan': False}]}

    wifi.remove_configured_wifi_network(0)

    end_response = wifi.get_all_wifi_networks()

    assert end_response == {'values': []}
