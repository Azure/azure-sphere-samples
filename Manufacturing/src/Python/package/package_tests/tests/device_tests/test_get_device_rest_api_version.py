# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import device
from packaging import version
from tests.helpers import utils


def test__get_device_rest_api_version__returns_correct():
    """Tests if getting the device rest api version returns the expected version in the expected format."""
    response = device.get_device_rest_api_version()

    rest_api_version_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'REST-API-Version': {'REST-API-Version': {'type': 'string'}}
        }
    }

    assert utils.validate_json(response, rest_api_version_schema)

    assert type(version.parse(
        response["REST-API-Version"])).__name__ == "Version"
