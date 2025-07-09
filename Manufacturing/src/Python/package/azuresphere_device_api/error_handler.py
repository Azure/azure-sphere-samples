# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from typing import Dict

import requests.models
from requests.exceptions import JSONDecodeError

from azuresphere_device_api.exceptions import (DeviceError, InvalidJsonError,
                                               UnknownDeviceError)

# pylint: disable=line-too-long
# fmt: off
ERROR_MESSAGE_MAP_DICT: Dict[int, str] = {
    1048578: "Application manifest must not include a policy field. See https://aka.ms/AzureSphereAppManifest for more information.",
    1048579: "Application manifest must not include one of the specified capabilities. See https://aka.ms/AzureSphereAppManifest for more information.",
    1048591: "Could not execute application; the application image is not valid.",
    1048601: "Application manifest requested pins that are already in use",
    1048596: "Operation not permitted; check if application development capability is enabled ('azsphere device enable-development')",
    2097159: "Application manifest must not include one of the specified capabilities. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097153: "Could not parse application manifest JSON. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097155: "Application manifest must contain 'EntryPoint' string field. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097156: "Application manifest must contain 'Schema' string field. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097157: "Application manifest must contain 'CmdArgs' string array. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097158: "Application manifest must contain 'Capabilities' object. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097161: "Application manifest 'Capabilities' field 'AllowedConnections' or 'AllowedApplicationConnections' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097162: "Application manifest 'Capabilities' field 'Gpio' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097165: "Could not parse peripheral capability field from application manifest. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097167: "Application manifest 'Capabilities' field 'WifiConfig' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097168: "Application manifest 'EntryPoint' field is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097169: "Application manifest 'SchemaVersion' field is not compatible. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097170: "Application manifest requests a UART that is not available.",
    2097173: "Application manifest must not include a policy field. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097174: "Application manifest 'EntryPoint' field does not exist. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097175: "Application manifest 'EntryPoint' is not a file. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097176: "Application manifest 'Capabilities' field 'DeviceAuthentication' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097177: "Application manifest is missing from image",
    2097178: "Application manifest specifies an unknown application type. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097180: "Application manifest 'Capabilities' field 'DeviceAuthentication' is not available for use. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097181: "Application manifest 'Capabilities' field 'DeviceAuthentication' is too long. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097182: "Application manifest 'Capabilities' field 'SystemTime' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097183: "Application manifest 'Capabilities' field 'MutableStorage' must be a JSON object type. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097184: "Application manifest 'Capabilities' field 'MutableStorage' must include 'SizeKB' field. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097185: "Application manifest 'Capabilities' field 'MutableStorage' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097186: "Application manifest 'Capabilities' field 'MutableStorage' specifies a 'SizeKB' that is too large. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097187: "Application manifest specifies peripherals that use the same underlying pin",
    2097188: "Application manifest 'Capabilities' field 'AllowedTcpServerPorts' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097189: "Application manifest 'Capabilities' field 'AllowedUdpServerPorts' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097190: "Application manifest 'Capabilities' field 'AllowedTcpServerPorts' has an invalid value. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097191: "Application manifest 'Capabilities' field 'AllowedUdpServerPorts' has an invalid value. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097192: "Application manifest 'Capabilities' field 'TimeSyncConfigParseError' has an invalid value. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097193: "Application manifest 'Capabilities' field 'DhcpService' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097194: "Application manifest 'Capabilities' field 'SntpService' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097195: "Application manifest 'Capabilities' field 'AllowedApplicationConnections' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097196: "Application manifest 'Capabilities' field 'CertStore' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097197: "Application manifest 'Capabilities' field 'EnterpriseWifiConfig' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097198: "Application manifest 'Capabilities' field 'SystemEventNotifications' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097199: "Application manifest 'Capabilities' field 'SoftwareUpdateDeferral' is not parseable. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097200: "Application manifest 'Capabilities' field 'PowerControls' is not parseable: it isn't an array of PowerControls, or it contains duplicate entries. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097201: "Application manifest 'Capabilities' field 'PowerControls' is empty. See https://aka.ms/AzureSphereAppManifest for more information.",
    2097202: "Application manifest 'Capabilities' field 'PowerControls' has an invalid value. See https://aka.ms/AzureSphereAppManifest for more information.",
    7340047: "Image not trusted by device; check if application development capability is enabled ('azsphere device enable-development')",
    7340050: "Image could not be staged; check that it is a valid image package",
    7340056: "Installation failed; check if application development capability is enabled ('azsphere device enable-development')",
    7340069: "Permission denied; check if application development capability is enabled ('azsphere device enable-development')",
    7340071: "Over-the-air updating of the device is in progress and sideloading is temporarily disabled. Please retry later.",
    7340079: "Image has an invalid component ID. Re-package your image and retry.",
    3145729: "Application already exists on the device",
    3145735: "A reboot is required",
    3145738: "Cannot start application in debug mode; Debugging server is not present on device; try running 'azsphere device enable-development'.",
    4194306: "The application has already been deployed to this device",
    13:      "Permission denied; application development capability may be required ('azsphere device enable-development')",
    5242883: "Application manifest requests a GPIO that is not defined on this device. See https://aka.ms/AzureSphereAppManifest for more information.",
    5242884: "Application manifest requests a GPIO that is already in use by another application",
    5242887: "Application manifest contains a peripheral that is not defined on this device. See https://aka.ms/AzureSphereAppManifest for more information.",
    5242888: "Application manifest requests a peripheral that is already in use by another application",
    5242895: "Application manifest requests a GPIO block that is already in use by another application",
    5242896: "Application manifest requests a PWM block that is already in use by another application",
    14680076: "Application is not present",
    14680081:
    "Image not trusted by device; check if application development capability is enabled ('azsphere device enable-development')",
    14680082: "Insufficient free space on the device",
    13631489: "Invalid interface. Possible invalid parameter. See https://aka.ms/AzureSphereEAPTLS/Setup for further information on setting up an EAP-TLS network connection.",
    13631490: "Network already exists",
    13631491: "Network not found",
    13631492: "Network not connected",
    13631496: "Invalid network properties provided. See https://aka.ms/AzureSphereEAPTLS/Setup for further information on setting up an EAP-TLS network connection.",
    13631499: "The network could not be found in the list of configured networks.",
    13631503: "SSID is too long",
    13631504: "Key is too long",
    13631505: "The Wi-Fi interface is still initializing. Please try again.",
    13631506: "Wi-Fi interface is disabled",
    13631507: "Insufficient space to store the network.",
    13631510: "There are not enough resources to add additional Wi-Fi networks.",
    13631511: "Configuration name already exists.",
    13631512: "Client identifier is missing.",
    13631513: "PSK is too short.",
    10485761: "Possible invalid parameter entry or authentication failure. See https://aka.ms/AzureSphereEAPTLS/Setup for further information on setting up an EAP-TLS network connection.",
    10485765: "Device is busy and refused request",
    10485766: "Unable to retrieve mutable storage quota for the requested component.",
    10485772: "Certificate identifier too long.",
    10485776: "Configuration name too long. Configuration name must be 1-16 characters in length.",
    38797314: "The supplied certificate identifier is invalid. Certificate identifiers may only consist of alphanumeric characters or .-_ ",
    38797315: "Certificate contents are invalid. Please supply a valid base64-encoded certificate.",
    38797316: "Certificate identifier too long. Certificate identifiers must be 1-16 characters in length.",
    38797319: "No certificate with that identifier could be found on the device.",
    38797323: "Unable to add certificate. Certificate store is out of space.",
    38797324: "Certificate type is invalid. Certificate type must be 'client' or 'rootca'.",
    38797325: "Public Certificate content is invalid.",
    38797326: "Public Certificate content is too long.",
    38797327: "Private Key content is invalid.",
    38797328: "Private Key content is too long.",
    38797329: "Private Key is not allowed for this certificate type.",
    38797330: "Private Key password is too long.",
    38797331: "Private Key password is not allowed for this certificate type.",
    38797332: "Certificate and Private Key pair do not match.",
    38797333: "Private Key password provided for unencrypted key.",
    15728644: "The content provided is too large.",
    31457286: "Invalid hardware address.",
    31457294: "Invalid interface.",
    31457295: "Operation not permitted on interface.",
    15728642: "Invalid request URI."
}

def _get_error_message(error: int) -> str:
    """Gets the error message from the dict."""
    if error not in ERROR_MESSAGE_MAP_DICT:
        raise UnknownDeviceError(f"Unknown device error: {error}")

    return ERROR_MESSAGE_MAP_DICT[error]

def handle_status_code_errors(response: requests.models.Response) -> Exception:
    """Checks the status code and raised appropriate exceptions."""

    status_code = response.status_code
    detail_msg = ""

    if len(response.content):
        try:
            json_response = response.json()
            detail_msg = _get_error_message(json_response['error'])
        except JSONDecodeError as ex:
            raise InvalidJsonError(
                "ERROR: The device returned a response which could not be parsed.") from ex

    if status_code == 400:
        raise DeviceError(f"ERROR: An invalid request was made to the device. {detail_msg}")

    if status_code == 403:
        raise DeviceError("ERROR: You do not have permission to perform this operation on this "
                          f"device. {detail_msg}")

    if status_code in [404, 412]:
        raise DeviceError(f"ERROR: This resource is unavailable on this device. {detail_msg}. Is your device up to date?")

    if status_code == 409:
        raise DeviceError("ERROR: The device could not perform this request due to the resource "
                          f"being in a conflicting state. {detail_msg}")

    if status_code == 415:
        raise DeviceError("ERROR: The media type provided for this operation is unsupported. "
                          f"{detail_msg}")

    if status_code == 500:
        raise DeviceError(f"ERROR: An internal device error occurred. {detail_msg}")

    message = (
        f"This API failed due to an unexpected HTTP error, with status code "
        f"'{status_code}', from the device."
    )
    raise UnknownDeviceError(message)
