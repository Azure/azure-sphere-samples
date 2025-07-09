# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
from functools import wraps
from packaging import version
from azuresphere_device_api.exceptions import DeviceError

_CURRENT_DEVICE_API_VERSION = ""


def set_current_device_api_version(device_api_version: str) -> None:
    """
    Modifies the current device api version used in the since_device_api_version check.
    :param: device_api_version the new DeviceAPI to use when validating compatibility. 
    :returns: None
    @note This is an internal function that should not be used except by this library.
    """
    print("setting current device api version: ", device_api_version)
    global _CURRENT_DEVICE_API_VERSION
    _CURRENT_DEVICE_API_VERSION = device_api_version


def since_device_api_version(since_version: str):
    """
    A decorator that ensures the current device is able to respond to an api.
    It checks the provided version against the device API version provided by a device.
    :param: since_version the DeviceAPI version at which the api is availabe from
    :returns: the original function on success, or raises a DeviceError
    """
    def version_decorator(func):
        @ wraps(func)
        def version_validator(*args, **kwargs):
            if _CURRENT_DEVICE_API_VERSION == "":
                from azuresphere_device_api.device import get_device_rest_api_version
                get_device_rest_api_version()

            if version.parse(since_version) > version.parse(_CURRENT_DEVICE_API_VERSION):
                raise DeviceError(
                    f" The current device does not support {func.__name__}. Required DeviceAPI version: {since_version}. DeviceAPI version reported by device: {_CURRENT_DEVICE_API_VERSION} ")
            return func(*args, **kwargs)
        return version_validator
    return version_decorator
