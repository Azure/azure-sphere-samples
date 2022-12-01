# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import wifi
from packaging import version
from azuresphere_device_api.validation import get_current_device_api_version
import pytest

def test__change_wifi_interface__true_returns_empty():
    """Tests if setting reload_config to True returns an empty json response."""

    response = wifi.change_wifi_interface_state(True)

    assert response == {}

def test__change_wifi_interface__false_returns_empty():
    """Tests if setting reload_config to False returns an empty json response."""

    response = wifi.change_wifi_interface_state(False)

    assert response == {}

def test__reload_config__true_returns_empty():
    """Tests if setting reload_config to True returns an empty json response."""

    response = wifi.set_wifi_interface_reload_configuration(True)

    assert response == {}

def test__reload_config__false_returns_empty():
    """Tests if setting reload_config to False returns an empty json response."""

    response = wifi.set_wifi_interface_reload_configuration(True)

    assert response == {}

def test__power_savings_enable__true_returns_empty():
    """Tests if setting wifi_power_savings to True returns an empty json response."""

    if version.parse(get_current_device_api_version()) >= version.parse("4.6.0"):
        response = wifi.change_wifi_interface_state(False, True)
        assert response == {}
    else:
        with pytest.raises(Exception) as e_info:
            wifi.change_wifi_interface_state(False, True)


def test__power_savings_enable__false_returns_empty():
    """Tests if setting wifi_power_savings to False returns an empty json response."""

    response = wifi.change_wifi_interface_state(False, False)

    assert response == {}

def test__change_all__true_returns_empty():
    """Tests if setting wifi_power_savings and reload_config to True returns an empty json response."""

    if version.parse(get_current_device_api_version()) >= version.parse("4.6.0"):
        response = wifi.change_wifi_interface_state(True, True)
        assert response == {}
    else:
        with pytest.raises(Exception) as e_info:
            wifi.change_wifi_interface_state(True, True)

def test__change_wifi_interface__enable_functions():
    """Tests if setting wifi_power_savings has any effect."""

    if version.parse(get_current_device_api_version()) >= version.parse("4.6.0"):
        response = wifi.change_wifi_interface_state(False, True)
        assert response == {}
        response = wifi.get_wifi_interface_state()
        assert response["powerSavingsState"] == "enabled"
        response = wifi.change_wifi_interface_state(False, False)
        assert response == {}
        response = wifi.get_wifi_interface_state()
        assert response["powerSavingsState"] == "disabled"
    else:
        with pytest.raises(Exception) as e_info:
            wifi.change_wifi_interface_state(True, True)

def test__power_savings__enable_functions():
    """Tests if setting wifi_power_savings has any effect."""

    if version.parse(get_current_device_api_version()) >= version.parse("4.6.0"):
        response = wifi.set_wifi_interface_power_savings(True)
        assert response == {}
        response = wifi.get_wifi_interface_state()
        assert response["powerSavingsState"] == "enabled"
        response = wifi.set_wifi_interface_power_savings(False)
        assert response == {}
        response = wifi.get_wifi_interface_state()
        assert response["powerSavingsState"] == "disabled"
    else:
        with pytest.raises(Exception) as e_info:
            wifi.change_wifi_interface_state(True, True)

