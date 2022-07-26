# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import time

from microsoft_azure_sphere_deviceapi import device
from tests.helpers import utils


def test__get_device_status__returns_correct_format():
    """Tests if getting the device status returns a response in the correct format."""
    response = device.get_device_status()

    uptime_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'uptime': {'uptime': {'type': 'integer'}}
        }
    }

    assert utils.validate_json(response, uptime_schema)


def test__get_device_status__uptime_increases_with_later_calls():
    """Tests if getting the device status at later points increases the 'uptime'."""
    start_time = device.get_device_status()["uptime"]

    time.sleep(1)

    end_time = device.get_device_status()["uptime"]

    assert start_time < end_time
