# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


from microsoft_azure_sphere_deviceapi import network


def test__failed_connections__fail_connection_returns_non_empty_list():
    """Tests if calling get all failed network connections returns that none have occurred."""
    response = network.get_all_failed_network_connections()

    assert {'values': []} == response
