# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import wifi
from tests.helpers import utils


def test__wifi_scan_returns_valid_format():
    """Tests if get wifi scan returns wifi networks of the expected format."""
    values_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'values': {'type': 'array'}
        }
    }

    wifi_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'bssid': {'type': 'string'},
            'freq': {'type': 'integer'},
            'signal_level': {'type': 'integer'},
            'ssid': {'type': 'string'},
            'securityState': {'type': 'string'}
        }
    }
    response = wifi.get_wifi_scan()

    assert utils.validate_json(response, values_schema)

    for net in response["values"]:
        assert utils.validate_json(net, wifi_schema)
