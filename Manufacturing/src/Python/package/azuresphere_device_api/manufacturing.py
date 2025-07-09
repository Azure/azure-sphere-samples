# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import azuresphere_device_api.utils as utils
from azuresphere_device_api.exceptions import ValidationError
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['get_device_manufacturing_state', 'set_device_manufacturing_state']


@since_device_api_version("3.1.0")
def get_device_manufacturing_state() -> dict:
    """Makes a REST "GET" request to get the manufacturing state of the attached device.

    :return: The manufacturing state of the device on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("device/manufacturing_state")


@since_device_api_version("3.1.0")
def set_device_manufacturing_state(manufacturing_state: str) -> dict:
    """Makes a REST "PUT" request to update the manufacturing state of the attached device.

    :param manufacturing_state: The manufacturing state of the attached device.
    :type manufacturing_state: str
    :accepted-parameters manufacturing_state: DeviceComplete, Module1Complete
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    valid_settable_manufacturing_states = ["DeviceComplete", "Module1Complete"]

    if manufacturing_state not in valid_settable_manufacturing_states:
        raise ValidationError(
            "ERROR: Cannot set manufacturing state, manufacturing state supplied is invalid.")

    return utils.put_request(
        "device/manufacturing_state", {
            "manufacturingState": manufacturing_state}
    )
