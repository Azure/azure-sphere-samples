# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import re
from sys import platform

from azuresphere_device_api import utils
from azuresphere_device_api.exceptions import ValidationError


def set_active_device_ip_address(ip_address: str) -> None:
    """Sets the device IP address used in all requests.

    :param ip_address: The device IP address to use.
    :type ip_address: string
    """
    if platform in ("linux", "linux2") and ip_address != '192.168.35.2':
        raise ValidationError(
            f"ERROR: Cannot set active device IP address {ip_address} on Linux. Linux does not have multi-board support.")

    regex_pattern = '^192.168.35.\\b([2-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\b$'
    pattern_matches = re.findall(regex_pattern, ip_address)

    if not pattern_matches:
        raise ValidationError(
            f"ERROR: Cannot set active device IP address {ip_address}, range is 192.168.35.2-192.168.35.255")

    utils.set_device_ip_address(ip_address)


def get_active_device_ip_address() -> str:
    """Returns the device IP address currently used in all requests.

    :returns: A string containing the current IP address.
    :rtype: str
    """
    return utils.get_device_ip_address()


def get_attached_devices() -> list:
    """Makes a "GET" request to list the attached devices.

    :return: List of attached devices on success. An exception will be thrown on error.
    message.
    :rtype: Tuple(int, dict[str, str])
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if platform == "win32":
        return utils.get_request("api/service/devices", api_type=utils.AzureSphereDeviceApiRequestType.LOCAL_DEVICE_URL)

    if platform in ("linux", "linux2"):
        return _list_linux_devices()

    raise ValidationError(
        "ERROR: Cannot get attached devices, unsupported operating system.")


def _linux_netifaces_ioctl():
    """
    This function retrieves the network interfaces from ioctl on linux.
    """
    import socket
    import fcntl
    import struct
    import array
    MAX_BYTES = 4096
    SIOCGIFCONF = 0x8912

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    names = array.array('B', MAX_BYTES * b'\0')

    ioctl_buff_addr, _ioctl_buff_length = names.buffer_info()
    mutable_byte_buffer = struct.pack('iL', MAX_BYTES, ioctl_buff_addr)
    # query ioctl (io control) for interface names
    mutated_byte_buffer = fcntl.ioctl(
        sock.fileno(), SIOCGIFCONF, mutable_byte_buffer)
    ioctl_byte_count, _names_address_out = struct.unpack(
        'iL', mutated_byte_buffer)

    ioctl_buff = names.tobytes()
    ioctl_buff[:ioctl_byte_count]
    ifaces = {}

    # each ioctl entry is 40 bytes long
    for i in range(0, ioctl_byte_count, 40):
        # the first 16 bytes are the name
        name = str(ioctl_buff[i: i+16].strip(b'\0'), encoding="utf8")
        # bytes 20-24 are ip address octets.
        # decode as human readable string.
        ip = '.'.join([str(octet) for octet in ioctl_buff[i+20:i+24]])
        ifaces[name] = ip

    return ifaces


def _list_linux_devices() -> list:
    """Linux method of getting attached devices.

    :return: List of attached devices on success.
    :rtype: Tuple(int, dict[str, str])
    """
    devices = []
    if "sl0" in _linux_netifaces_ioctl():
        devices = [{"IpAddress": "192.168.35.2", "DeviceConnectionPath": ""}]
    return devices
