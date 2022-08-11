# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import network


def test__delete_proxy__delete_when_no_proxy_returns_empty_json_response():
    """Tests if attempting to delete a proxy when there isn't one, returns an empty response."""
    response = network.delete_network_proxy()

    assert {} == response


def test__delete_proxy__delete_when_proxy_returns_empty_json_response(fix_clean_proxies):
    """Tests if calling delete when there is a proxy, returns an empty response."""
    network.configure_proxy(True, "example.com", 8081, [], "anonymous")

    response = network.delete_network_proxy()

    assert {} == response


def test__delete_proxy__delete_proxy_deletes_proxy(fix_clean_proxies):
    """Tests if attempting to delete a proxy when there is a proxy, deletes the proxy."""
    network.configure_proxy(True, "example.com", 8081, [], "anonymous")

    start_response = network.get_network_proxy()

    assert start_response == {'address': 'example.com', 'authenticationType': 'anonymous',
                              'enabled': True, 'noProxyAddresses': [], 'port': 8081}

    network.delete_network_proxy()

    end_response = network.get_network_proxy()

    assert end_response == {}
