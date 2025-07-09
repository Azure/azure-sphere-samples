# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from typing import Literal

from azuresphere_device_api import utils
from azuresphere_device_api.exceptions import ValidationError
from azuresphere_device_api.validation import since_device_api_version

__all__ = ['add_wifi_network', 'change_wifi_network_config', 'change_wifi_interface_state', 'get_all_wifi_networks',
           'get_configured_wifi_network', 'get_wifi_interface_state', 'get_wifi_scan', 'remove_configured_wifi_network']


def add_wifi_network(
    ssid: str,
    security_state: Literal["eaptls", "open", "psk"],
    config_name=None,
    config_state: Literal["enabled", "disabled"] = None,
    target_scan=None,
    psk=None,
    client_identity=None,
    client_cert_store_identifier=None,
    root_ca_cert_store_identifier=None,
) -> dict:
    """Makes a "POST" request to add a Wi-Fi network on the attached device.

    :param ssid: The SSID of the new network.
    :type ssid: str
    :param security_state: The network type.
    :type security_state: str
    :accepted-values security_state: eaptls, open, psk
    :param config_name: A string value (up to 16 characters) that specifies the name for this
    network.
    :type config_name: str
    :param config_state: If the network is enabled or disabled.
    :type config_state: str
    :accepted-values config_state: enabled, disabled
    :param target_scan: Attempt to connect to an SSID even if not advertised.
    :type target_scan: str
    :param psk: The WPA/WPA2 PSK for the new network. Do not set this if connecting to an open
    network.
    :type psk: str
    :param client_identity: user@domain [EAP-TLS] ID recognized for authentication by this
    networks RADIUS server. Required for some EAP-TLS networks.
    :type client_identity: str
    :param client_cert_store_identifier: [EAP-TLS] A string value (up to 16 characters) that
    identifies the client certificate (containing both the public and private key). Required to set
    up an EAP-TLS network.
    :type client_cert_store_identifier: string
    :param root_ca_cert_store_identifier: [EAP-TLS] A string value (up to 16 characters) that
    identifies the networks root CA certificate for EAP-TLS networks where the device authenticates
    the server.
    :type root_ca_cert_store_identifier: string

    :return: The details of the newly configured network on success. An exception will be thrown on error.
    :rtype: Tuple(int, dict[str, str])
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not ssid or ssid.isspace():
        raise ValidationError(
            "ERROR: Cannot add wifi network config, invalid SSID.")

    valid_config_states = ["enabled", "disabled"]

    if not config_state or config_state.isspace() or config_state not in valid_config_states:
        raise ValidationError(
            "ERROR: Cannot add wifi network config, invalid config state.")

    valid_security_states = ["eaptls", "open", "psk"]

    if not security_state or security_state.isspace() or security_state not in valid_security_states:
        raise ValidationError(
            "ERROR: Cannot add wifi network config, invalid network type.")

    body = {}

    if security_state == "eaptls":
        body = _add_wifi_network_eaptls(
            ssid,
            security_state,
            config_name,
            config_state,
            target_scan,
            client_identity,
            client_cert_store_identifier,
            root_ca_cert_store_identifier,
        )
    elif security_state == "open":
        body = _add_wifi_network_open(
            ssid, security_state, config_name, config_state, target_scan
        )
    elif security_state == "psk":
        body = _add_wifi_network_psk(
            ssid, security_state, config_name, config_state, target_scan, psk
        )

    if "error" in body.keys():
        return body

    return utils.post_request("wifi/config/networks", body)


def _add_wifi_network_eaptls(
    ssid: str,
    security_state: str,
    config_name: str,
    config_state: str,
    target_scan: bool,
    client_identity: str,
    client_cert_store_identifier: str,
    root_ca_cert_store_identifier: str,
) -> dict:
    """Creates a JSON string given a set of parameters for the Eaptls network type.

    :param ssid: The SSID of the new network.
    :type ssid: string
    :param security_state: The network type.
    :type security_state: string
    :param config_name: A string value (up to 16 characters) that specifies the name for this
    network.
    :type config_name: string
    :param config_state: If the network is enabled or disabled.
    :type config_state: string
    :param target_scan: Attempt to connect to an SSID even if not advertised.
    :type target_scan: string
    :param client_identity: <user@domain> [EAP-TLS] ID recognized for authentication by this
    networks RADIUS server. Required for some EAP-TLS networks.
    :type client_identity: string
    :param client_cert_store_identifier: [EAP-TLS] A string value (up to 16 characters) that
    identifies the client certificate (containing both the public and private key). Required to set
    up an EAP-TLS network.
    :type client_cert_store_identifier: string
    :param root_ca_cert_store_identifier: [EAP-TLS] A string value (up to 16 characters) that
    identifies the networks root CA certificate for EAP-TLS networks where the device authenticates
    the server.
    :type root_ca_cert_store_identifier: string
    :return: The JSON string on success, or null on failure.
    :rtype: dict[str, str]

    """
    if client_cert_store_identifier is None:
        raise ValidationError(
            "ERROR: Cannot add WiFi network for EAP-TLS, client cert store identifier is null or empty.")

    body = {
        "ssid": ssid,
        "securityState": security_state,
        "clientCertStoreIdentifier": client_cert_store_identifier,
    }

    if config_name:
        body["configName"] = config_name
    if config_state:
        body["configState"] = config_state
    if target_scan:
        body["targetScan"] = target_scan
    if client_identity:
        body["clientIdentity"] = client_identity
    if root_ca_cert_store_identifier:
        body["rootCaCertStoreIdentifier"] = root_ca_cert_store_identifier

    return body


def _add_wifi_network_open(
    ssid: str,
    security_state: str,
    config_name: str,
    config_state: str,
    target_scan: bool,
):
    """Creates a JSON string given a set of parameters for the Open network type.

    :param ssid: The SSID of the new network.
    :type ssid: string
    :param security_state: The network type.
    :type security_state: string
    :param config_name: A string value (up to 16 characters) that specifies the name for this
    network.
    :type config_name: string
    :param config_state: If the network is enabled or disabled.
    :type config_state: string
    :param target_scan: Attempt to connect to an SSID even if not advertised.
    :type target_scan: string
    :return: The JSON string on success, or null on failure.
    :rtype: dict[str, str]

    Returns:
        _type_: _description_
    """
    body = {
        "ssid": ssid,
        "securityState": security_state,
    }

    if config_name:
        body["configName"] = config_name
    if config_state:
        body["configState"] = config_state
    if target_scan:
        body["targetScan"] = target_scan

    return body


def _add_wifi_network_psk(
    ssid: str,
    security_state: str,
    config_name: str,
    config_state: str,
    target_scan: bool,
    psk: str,
):
    """Creates a JSON string given a set of parameters for the psk network type.

    :param ssid: The SSID of the new network.
    :type ssid: string
    :param security_state: The network type.
    :type security_state: string
    :param config_name: A string value (up to 16 characters) that specifies the name for this
    network.
    :type config_name: string
    :param config_state: If the network is enabled or disabled.
    :type config_state: string
    :param target_scan: Attempt to connect to an SSID even if not advertised.
    :type target_scan: string
    :param psk: The WPA/WPA2 PSK for the new network. Do not set this if connecting to an open
    network.
    :type psk: string
    :return: The JSON string on success, or null on failure.
    :rtype: dict[str, str]

    Returns:
        _type_: _description_
    """
    if psk is None:
        raise ValidationError(
            "ERROR: Cannot add wifi network for psk, psk is null or empty.")

    body = {"ssid": ssid, "securityState": security_state, "psk": psk}

    if config_name:
        body["configName"] = config_name
    if config_state:
        body["configState"] = config_state
    if target_scan:
        body["targetScan"] = target_scan

    return body


def change_wifi_network_config(
    network_id: int, config_state: Literal["unknown", "enabled", "disabled", "temp-disabled"], psk: str
) -> dict:
    """Makes a "PATCH" request to modify a specific Wi-Fi network configuration.

    :param network_id: The ID of the desired network.
    :type network_id: int
    :param config_state: The state of the configured network.
    :type config_state: str
    :accepted-values config_state: unknown, enabled, disabled, temp-disabled
    :param psk: The WPA/WPA2 PSK for the network.
    :type psk: string

    :return: The updated Wi-Fi network configuration on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """

    valid_config_states = ["unknown", "enabled", "disabled", "temp-disabled"]

    if not config_state or config_state not in valid_config_states:
        raise ValidationError(
            "ERROR: Cannot change Wi-Fi network config, config state is invalid.")

    if not psk:
        raise ValidationError(
            "ERROR: Cannot change Wi-Fi network config, psk is null or empty.")

    return utils.patch_request(
        f"wifi/config/networks/{network_id}", {
            "configState": config_state, "psk": psk}
    )


def change_wifi_interface_state(reload_config: bool) -> dict:
    """Makes a "PATCH" request to enable/disable a Wi-Fi network on the attached device.

    :param reload_config: Your desired boolean state for reloadConfig.
    :type reload_config: boolean
    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.patch_request("wifi/interface", {"reloadConfig": reload_config})


def get_all_wifi_networks() -> dict:
    """Makes a "GET" request to get the current Wi-Fi configurations for the attached device.

    :return: All the current Wi-Fi configurations on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("wifi/config/networks")


def get_configured_wifi_network(network_id: int) -> dict:
    """Makes a "GET" request to get a specific Wi-Fi network configuration.

    :param network_id: The ID of the desired network.
    :type network_id: int
    :return: The Wi-Fi network configuration on success. An exception will be thrown on error.
    :rtype: dict
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if network_id < 0:
        raise ValidationError(
            "ERROR: Cannot get a Wi-Fi network configuration, network_id is null or empty.")

    return utils.get_request(f"wifi/config/networks/{network_id}")


def get_wifi_interface_state() -> dict:
    """Makes a "GET" request to get the state of the wireless interface on the attached device.

    :return:The status of the wireless interface on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("wifi/interface")


def get_wifi_scan() -> dict:
    """Makes a "GET" request to show Wi-Fi networks visible to the attached device.

    :return: The list of Wi-Fi networks visible to the attached device on success.
             An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("wifi/scan")


@since_device_api_version("3.0.0")
def remove_configured_wifi_network(network_id: int) -> dict:
    """Makes a "DELETE" request to delete a specific Wi-Fi network configuration.

    :param network_id: The ID of the network.
    :type networkId: int

    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if network_id < 0:
        raise ValidationError(
            "ERROR: Cannot delete a Wi-Fi network configuration, network_id is null or empty.")

    return utils.delete_request(f"wifi/config/networks/{network_id}")
