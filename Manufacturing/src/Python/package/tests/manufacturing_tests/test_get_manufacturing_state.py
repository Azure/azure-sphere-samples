# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import manufacturing
from tests.helpers import utils


def test__get_manufacturing_state__returns_expected_state():
    """Tests if get manufacturing returns a response of the correct format."""
    response = manufacturing.get_device_manufacturing_state()

    valid_manufacturing_states = ["Blank", "Module1Complete", "DeviceComplete", "Unknown"]

    top_level_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'manufacturingState': {'type': 'string'}
        }
    }

    assert utils.validate_json(response, top_level_schema)

    assert response["manufacturingState"] in valid_manufacturing_states
