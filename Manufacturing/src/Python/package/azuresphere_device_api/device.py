# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


from azuresphere_device_api import utils
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['clear_error_report_data', 'get_device_rest_api_version', 'get_device_os_version', 'get_device_security_state',
           'get_device_status', 'get_diagnostic_log', 'get_error_report_data', 'restart_device']


def clear_error_report_data() -> dict:
    """Makes a "DELETE" request to clear a device's error report data.

    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.delete_request("telemetry")


def get_device_rest_api_version() -> dict:
    """Makes a "GET" request to get the device rest api version.

    :return: The api version number on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    response = utils.get_request("status",
                                 utils.AzureSphereDeviceApiRequestType.DEVICE_REST_API_VERSION)

    return {"REST-API-Version": response["REST-API-Version"]}


@since_device_api_version("4.5.0")
def get_device_os_version() -> dict:
    """Makes a "GET" request to get the device OS version.

    :return: The OS version number on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("osversion")


def get_device_security_state() -> dict:
    """Makes a "GET" request to get the security state of the device.

    :return: The security state on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("device/security_state")


def get_device_status() -> dict:
    """Makes a "GET" request to retrieve the uptime of a device.

    :return: The device status on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("status")


@since_device_api_version("1.2.0")
def get_diagnostic_log() -> dict:
    """Makes a "GET" request to get diagnostics logs of the attached device.

    :return: The diagnostics log binary on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("log")


@since_device_api_version("3.1.0")
def get_error_report_data() -> bytes:
    """Makes a "GET" request to get a device's error report data.

    :return: The device error report data on success. An exception will be thrown on error.
    :rtype: bytes
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("telemetry")


def restart_device() -> dict:
    """Makes a "POST" request to restart the attached device.

    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.post_request_no_body("restart")
