# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
from azuresphere_device_api import device, utils, exceptions
import pytest


def test__query_without_device__raises_exception():
    """Tests if querying a device that does not exist raises an exception"""
    utils.set_device_ip_address("192.168.35.51")
    with pytest.raises(exceptions.DeviceError, match="Device connection timed out for") as e_info:
        device.get_device_status()
    # restore default device IP.
    utils.set_device_ip_address("192.168.35.2")
    assert e_info.type == exceptions.DeviceError
