# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import device
from tests.helpers import utils


def test__get_device_security_state__returns_correct_format():
    """Tests if getting the the device security state returns a response in the expected format."""
    response = device.get_device_security_state()

    uptime_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'uptime': {'securityState': {'type': 'string'}, 'deviceIdentifier': {'type': 'string'}, 'deviceIdentityPublicKey': {'type': 'string'}}
        }
    }

    assert utils.validate_json(response, uptime_schema)
