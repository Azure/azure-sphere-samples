# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import wifi


def test__change_interface_state__true_returns_empty():
    """Tests if changing the interface state to true returns an empty json response."""

    response = wifi.change_wifi_interface_state(True)

    assert response == {}


def test__change_interface_state__false_returns_empty():
    """Tests if changing the inteface state to False returns an empty json response."""

    response = wifi.change_wifi_interface_state(False)

    assert response == {}

