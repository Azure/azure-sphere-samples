# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import azuresphere_device_api.utils as utils

__all__ = ['get_images']


def get_images() -> dict:
    """Makes a REST "GET" request to list the images currently on the attached device.

    :return: All images running on the device on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("images")
