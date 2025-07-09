# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import os
from typing import Literal

import azuresphere_device_api.utils as utils
from azuresphere_device_api.exceptions import ValidationError
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['delete_image', 'install_images', 'stage_image']


def delete_image(component_id: str) -> dict:
    """Makes a "DELETE" request to remove a component from the device.
    This requires enabling development mode.

    :param component_id: The component id of the image to be deleted.
    :type component_id: string
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: Tuple(int, dict[str, str])
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not component_id or not utils.is_uuid(component_id):
        raise ValidationError(
            "ERROR: Cannot delete image, invalid component ID.")

    return utils.delete_request(f"app/image/{component_id}")


@since_device_api_version("3.0.0")
def install_images(app_control_mode: Literal["Auto", "Manual"] = "Auto") -> dict:
    """Makes a "POST" request to install all staged images on a device.
    For deploying unsigned images, this requires enabling development mode.

    :param app_control_mode: Determines if application starts automatically after installation,
    defaults to 'Auto'.
    :type app_control_mode: str
    :accepted-values app_control_mode: Auto, Manual
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: Tuple(int, dict[str, str])
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    valid_app_control_modes = ["Auto", "Manual"]

    if app_control_mode not in valid_app_control_modes:
        raise ValidationError(
            "ERROR: Cannot set the app status, invalid component ID.")

    return utils.post_request("update/install", {"appControlMode": app_control_mode})


@since_device_api_version("3.0.0")
def stage_image(image_path: str) -> dict:
    """Makes a "PUT" request to stage an image on a device.
    For deploying unsigned images, this requires enabling development mode.

    :param image_path: The absolute path to an image package.
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: Tuple(int, dict[str, str])
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not image_path:
        raise ValidationError(
            "ERROR: Cannot stage image, image path is null or empty.")

    if not os.path.isfile(image_path):
        raise ValidationError(
            "ERROR: Cannot stage image, image path is null or empty.")

    with open(image_path, "rb") as opened_image:
        return utils.put_request_octet_stream(
            "update/stage", opened_image.read()
        )
