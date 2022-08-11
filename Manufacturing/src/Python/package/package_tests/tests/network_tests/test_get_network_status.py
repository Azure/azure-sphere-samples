# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import network


def test__get_network_status__returns_expected_status():
    """Tests if getting the network status returns the expected values."""
    response = network.get_network_status()

    assert {'deviceAuthenticationIsReady': False,
            'networkTimeSync': 'incomplete', 'proxy': 'disabled'} == response
