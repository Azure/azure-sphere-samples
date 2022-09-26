# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, wifi


def test__get_wifi_network_non_existent_id_returns_not_found(fix_setup_networks):
    """Tests if getting a wifi network with a non existent id throws a device error."""
    with pytest.raises(exceptions.DeviceError, match="Network not found"):
        wifi.get_configured_wifi_network(0)


def test__get_wifi_network_existing_id_returns_ok(fix_setup_networks):
    """Tests if getting a network with an existing id returns the expected network configuration."""
    wifi.add_wifi_network("Test", "open", "", "enabled")

    response = wifi.get_configured_wifi_network(0)

    assert response == {'configState': 'enabled', 'connectionState': 'disconnected',
                        'id': 0, 'securityState': 'open', 'ssid': 'Test', 'targetedScan': False}
