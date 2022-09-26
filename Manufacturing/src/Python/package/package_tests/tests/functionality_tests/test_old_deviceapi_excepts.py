# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
from azuresphere_device_api import device, validation, exceptions
import pytest


def test__old_deviceapi_version__raises_exception():
    """Tests if querying a device that does not exist raises an exception"""
    validation.set_current_device_api_version("1.0.0")
    with pytest.raises(exceptions.DeviceError, match="The current device does not support get_device_os_version") as e_info:
        device.get_device_os_version()
    validation.set_current_device_api_version("")
    assert e_info.type == exceptions.DeviceError
