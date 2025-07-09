# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from typing import List, Literal

import azuresphere_device_api.utils as utils
from azuresphere_device_api.exceptions import ValidationError
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['configure_proxy', 'delete_network_proxy', 'get_all_failed_network_connections', 'get_all_network_interfaces', 'get_failed_network_connection',
           'get_network_firewall_ruleset', 'get_network_interface', 'get_network_proxy', 'get_network_status', 'set_network_interfaces']


def configure_proxy(
    enabled: bool,
    address: str,
    port: int,
    no_proxy_address: List[str],
    authentication_type: Literal["anonymous", "basic"],
    username=None,
    password=None,
) -> dict:
    """Makes a "POST" request to configure the network proxy on the attached device.

    :param enabled: Enable/disable network proxy on attached device.
    :type enabled: bool
    :param address: The network address of the proxy.
    :type address: str
    :param port: The port on the network proxy to be used.
    :type port: int
    :param no_proxy_address: Array of space-separated network addresses the device should avoid for
        proxy connection.
    :type no_proxy_address: list[str]
    :param authentication_type: If the proxy requires a user name and password, set this to basic,
        otherwise set it to anonymous.
    :type authentication_type: str
    :accepted-values authentication_type: anonymous, basic
    :param username: Optional parameter. Username used for proxy authentication in basic mode.
    :type username: str
    :param password: Optional parameter. Password used for proxy authentication in basic mode.
    :type password: str
    :return: The details of the supplied proxy configuration on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not address:
        raise ValidationError(
            "ERROR: Cannot configure proxy, address was null or empty.")

    valid_authentication_types = ["anonymous", "basic"]

    if authentication_type not in valid_authentication_types:
        raise ValidationError(
            "ERROR: Cannot configure proxy, invalid authentication type.")

    if username and password:

        if authentication_type == "anonymous":
            raise ValidationError(
                "ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.")

        # Basic type network proxy
        body = {
            "enabled": enabled,
            "address": address,
            "port": port,
            "noProxyAddresses": no_proxy_address,
            "authenticationType": authentication_type,
            "username": username,
            "password": password,
        }
        return utils.post_request("net/proxy", body)

    if authentication_type == "basic" or username or password:
        raise ValidationError(
            "ERROR: Cannot configure proxy, incorrect configuration of authentication type and proxy credentials.")

    # Anonymous type network proxy
    body = {
        "enabled": enabled,
        "address": address,
        "port": port,
        "noProxyAddresses": no_proxy_address,
        "authenticationType": authentication_type,
    }
    return utils.post_request("net/proxy", body)


def delete_network_proxy() -> dict:
    """Makes a "DELETE" request to delete the current network proxy.

    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.delete_request("net/proxy")


def get_all_failed_network_connections() -> dict:
    """Makes a "GET" request to get all the failed network connection attempts.

    :return: The failed connection attempts on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("wifi/diagnostics/networks")


@since_device_api_version("3.1.0")
def get_all_network_interfaces() -> dict:
    """Makes a "GET" request to get the status of all network interfaces.

    :return: The status of all network interfaces on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("net/interfaces")


def get_failed_network_connection(network_id: str) -> dict:
    """Makes a "GET" request to get a specific failed network connection attempt.

    :param network_id: The id of the network you would like the failed connection attempt of.
    :return: The failed network connection attempt on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request(f"wifi/diagnostics/networks/{network_id}")


@since_device_api_version("4.1.0")
def get_network_firewall_ruleset() -> dict:
    """Makes a "GET" request to get all network firewall rulesets.

    :return: All network firewall rulesets on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("net/firewall/rulesets")


@since_device_api_version("3.1.0")
def get_network_interface(network_interface_name: str) -> dict:
    """Makes a "GET" request to get the status of the named network interface.

    :param network_interface_name: The name of the network interface you would like to get the
    status of.
    :type network_interface_name: str
    :return: The status of the named network interface on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not network_interface_name:
        raise ValidationError(
            "ERROR: Cannot get network interface, input was null or empty.")

    return utils.get_request(f"net/interfaces/{network_interface_name}")


def get_network_proxy() -> dict:
    """Makes a "GET" request to get information about the current network proxy.

    :return: The details of the current network proxy on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("net/proxy")


@since_device_api_version("3.1.0")
def get_network_status() -> dict:
    """Makes a "GET" request to get the network status.

    :return: The status of the current network on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("net/status")


@since_device_api_version("3.1.0")
def set_network_interfaces(
    network_interface_name: str, interface_up: bool
) -> dict:
    """Makes a "PATCH" request to add/update a specific network interface's attributes.

    :param network_interface_name: The name of the network interface you would like to add/update.
    :type network_interface_name: string
    :param interface_up: If you would like the interface to be active or not.
    :type interface_up: boolean
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    if not network_interface_name:
        raise ValidationError(
            "ERROR: Cannot set network interface, network interface name was null or empty.")

    return utils.patch_request(
        f"net/interfaces/{network_interface_name}",
        {"interfaceUp": interface_up},
    )
