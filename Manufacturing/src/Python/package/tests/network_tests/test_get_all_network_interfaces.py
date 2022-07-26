# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


from microsoft_azure_sphere_deviceapi import network
from tests.helpers import utils


def test__get_all_network_interfaces__returns_correct_format():
    """Tests if getting all network interfaces, returns interfaces in the correct format."""
    response = network.get_all_network_interfaces()

    interface_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'interfaceName': {'type': 'string'},
            'interfaceUp': {'type': 'boolean'},
            'connectedToNetwork': {'type': 'boolean'},
            'ipAcquired': {'type': 'boolean'},
            'connectedToInternet': {'type': 'boolean'},
            'ipAddresses': {'type': 'array'},
            'hardwareAddress': {'type': 'string'},
            'ipAssignment': {'type': 'string'}
        }
    }

    for interface in response['interfaces']:
        assert utils.validate_json(interface, interface_schema)
