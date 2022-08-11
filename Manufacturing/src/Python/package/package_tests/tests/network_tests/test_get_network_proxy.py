# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import network


def test__get_network_proxy__returns_empty_json_response(fix_clean_proxies):
    """Tests if getting the network proxy when there isnt one returns an empty json response."""
    response = network.get_network_proxy()

    assert {} == response


def test__get_network_proxy__add_proxy_returns_proxy(fix_clean_proxies):
    """Tests if configuring a network proxy shows the proxy when attempting to get it."""
    network.configure_proxy(True, "example.com", 8081, [], "anonymous")

    response = network.get_network_proxy()

    assert {'address': 'example.com', 'authenticationType': 'anonymous',
            'enabled': True, 'noProxyAddresses': [], 'port': 8081} == response


def test__get_network_proxy__add_delete_proxy_returns_empty_json(fix_clean_proxies):
    """Tests if deleting a configured network proxy and then deleting it, doesnt then show up when attempting to get it."""
    network.configure_proxy(True, "example.com", 8081, [], "anonymous")

    network.delete_network_proxy()

    response = network.get_network_proxy()

    assert {} == response
