# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import time

import pytest
from azuresphere_device_api import exceptions, network, wifi


def test__set_interfaces__null_network_interface_throws_validation_error():
    """Tests if setting an interface with a null name throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot set network interface, network interface name was null or empty"):
        network.set_network_interfaces(None, False)
    network.set_network_interfaces("wlan0", True)


def test__set_interfaces__empty_network_interface_throws_validation_error():
    """Tests if setting an interface with an empty name throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot set network interface, network interface name was null or empty"):
        network.set_network_interfaces("", False)
    network.set_network_interfaces("wlan0", True)


def test__set_interfaces__changes_interface_up():
    """Tests if switching the interface state between true and false is possible."""
    network.set_network_interfaces("wlan0", True)

    network.set_network_interfaces("wlan0", False)
    _wait_for_interface_change(False)

    network.set_network_interfaces("wlan0", True)
    _wait_for_interface_change(True)


def _wait_for_interface_change(interface_state):
    """Helper function that waits for an interface state to change.

    :param interface_state: The state to wait for
    :type interface_state: str

    :return: True if the current interface state is our desired state, False if not observed this behaviour within timeout
    :rtype: bool
    """
    maxTime = 5000
    timeElapsed = 0
    while timeElapsed < maxTime:
        content = network.get_network_interface("wlan0")
        state = content["interfaceUp"]
        if state == interface_state:
            return True
        else:
            time.sleep(0.25)
            timeElapsed += 250

    return False


def test__set_interfaces__set_interface_to_false_makes_wifi_fail():
    """Tests if setting the network interface wlan0 causes wifi endpoints to not respond."""
    network.set_network_interfaces("wlan0", False)

    with pytest.raises(exceptions.DeviceError, match="ERROR: An internal device error occurred. Wi-Fi interface is disabled"):
        wifi.get_wifi_scan()

    with pytest.raises(exceptions.DeviceError, match="ERROR: An internal device error occurred. Wi-Fi interface is disabled"):
        wifi.get_all_wifi_networks()

    with pytest.raises(exceptions.DeviceError, match="ERROR: An internal device error occurred. Wi-Fi interface is disabled"):
        wifi.remove_configured_wifi_network(0)

    with pytest.raises(exceptions.DeviceError, match="ERROR: An internal device error occurred. Wi-Fi interface is disabled"):
        network.get_all_failed_network_connections()

    network.set_network_interfaces("wlan0", True)
