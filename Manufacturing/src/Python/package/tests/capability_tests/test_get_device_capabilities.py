# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import capabilities
from tests.helpers import utils


def test__device_capabilities__device_has_valid_format_capabilities():
    """Tests if getting the device capabilities shows that the expected capabilities are present and are in the correct format."""

    response_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'device_capabilities': {'type': 'array'}
        }
    }

    response = capabilities.get_device_capabilities()

    assert utils.validate_json(response, response_schema)
    assert all(isinstance(item, int) for item in response["device_capabilities"])
