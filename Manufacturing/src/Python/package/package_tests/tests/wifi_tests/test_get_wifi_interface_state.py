# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import wifi
from packaging import version
from azuresphere_device_api.validation import get_current_device_api_version
from tests.helpers import utils


def test__get_wifi_interface_state__returns_in_expected_format():
    """Tests if getting the wifi interface state returns an interface of the expected format."""

    state_schema = {'type': 'object', 'properties': {'configState': {'type': 'string'}, 'connectionState': {'type': 'string'}, 'securityState': {'type': 'string'}, 'mode': {
        'type': 'string'}, 'key_mgmt': {'type': 'string'}, 'wpa_state': {'type': 'string'}, 'address': {'type': 'string'}, 'id': {'type': 'integer'}}}

    if version.parse(get_current_device_api_version()) >= version.parse("4.6.0"):
        state_schema = {'type': 'object', 'properties': {'configState': {'type': 'string'}, 'connectionState': {'type': 'string'}, 'securityState': {'type': 'string'}, 'mode': {
        'type': 'string'}, 'key_mgmt': {'type': 'string'}, 'wpa_state': {'type': 'string'}, 'address': {'type': 'string'}, 'id': {'type': 'integer'}, 'powerSavingsState':{'type':'string'}}}

    response = wifi.get_wifi_interface_state()
    assert utils.validate_json(state_schema, response)
