# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from typing import Literal

from azuresphere_device_api import utils
from azuresphere_device_api.exceptions import ValidationError
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['get_app_quota', 'get_app_status',
           'get_memory_statistics', 'set_app_status']


@since_device_api_version("3.1.0")
def get_app_quota(component_id: str) -> dict:
    """Makes a "GET" request to show the storage quota and usage for a specific component on the
    attached device.

    :param component_id: The ID of the component to get the quota information for.
    :type component_id: string
    :return: The storage quota on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not component_id or not utils.is_uuid(component_id):
        raise ValidationError(
            "ERROR: Cannot get the app quota, invalid component ID.")

    return utils.get_request(f"app/quota/{component_id}")


@since_device_api_version("2.0.0")
def get_app_status(component_id: str) -> dict:
    """Makes a "GET" request to get the status of a component.

    :param component_id: The ID of the component to get the status for.
    :type component_id: string
    :return: The application state on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not component_id or not utils.is_uuid(component_id):
        raise ValidationError(
            "ERROR: Cannot get the app status, invalid component ID.")

    return utils.get_request(f"app/status/{component_id}")


def get_memory_statistics() -> dict:
    """Makes a "GET" request to show the memory statistics for applications on the attached device.

    :return: The memory statistics for applications on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("stats/memory/groups/applications")


@since_device_api_version("2.0.0")
def set_app_status(component_id: str, trigger: Literal["start", "startDebug", "stop"]) -> dict:
    """Makes a "PATCH" request to set the application status of a component.
    This requires enabling development mode.

    :param component_id: The ID of the component to set the status for.
    :type component_id: string
    :param trigger: The status of the application you would like to set.
    :type trigger: string
    :accepted-values trigger: start, startDebug, stop
    :return: The application state on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not component_id or not utils.is_uuid(component_id):
        raise ValidationError(
            "ERROR: Cannot set the app status, invalid component ID.")

    valid_trigger_values = ["start", "startDebug", "stop"]

    if not trigger or trigger not in valid_trigger_values:
        raise ValidationError(
            "ERROR: Cannot set the app status, invalid trigger.")

    return utils.patch_request(f"app/status/{component_id}", {"trigger": trigger})
