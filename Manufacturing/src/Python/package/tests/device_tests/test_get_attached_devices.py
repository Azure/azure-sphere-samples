# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import devices
from tests.helpers import utils


def test__get_attached_devices__returns_correct_format():
    """Getting the attached devices returns a list of items in the correct format."""
    response = devices.get_attached_devices()

    inner_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'IpAddress': {'type': 'string'},
            'DeviceConnectionPath': {'type': 'string'}
        }
    }

    for dev in response:
        assert utils.validate_json(dev, inner_schema)
