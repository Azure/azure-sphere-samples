# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import utils
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['get_device_capabilities']


@since_device_api_version("3.1.0")
def get_device_capabilities() -> dict:
    """Makes a "GET" request to show the current device capability configuration of
    the attached device.

    :return: The device capability configuration on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("device/capabilities")
