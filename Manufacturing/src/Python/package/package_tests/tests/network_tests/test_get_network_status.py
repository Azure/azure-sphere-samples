# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import network
from jsonschema import validate


def test__get_network_status__returns_expected_status():
    """Tests if getting the network status returns the expected values."""
    response = network.get_network_status()

    schema = {
        "type" : "object",
        "properties" : {
            "deviceAuthenticationIsReady" : {"type" : "boolean"},
            "networkTimeSync" : {"type" : "string"},
            "proxy" : {"type" : "string"},
        },
    }

    validate(instance=response, schema=schema)

    # no assert: the unit test will fail if something goes wrong

