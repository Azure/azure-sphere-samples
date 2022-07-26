# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import os
import time

import jsonschema
import requests
from jsonschema import validate
from microsoft_azure_sphere_deviceapi import certificate, image, sideload, wifi


def _get_path_to_mutable_storage() -> str:
    """Gets the path to the mutable storage image package.

    :return: The path to the image.
    :rytpe: string
    """
    return _get_file_from_helpers("images", "MutableStorage.imagepackage")


def _get_blink_path() -> str:
    """Gets the path to the blink image package.

    :return: The path to the image.
    :rytpe: string
    """
    return _get_file_from_helpers("images", "azure_sphere_blink.imagepackage")


def _get_test_client_cert() -> str:
    """Gets the path to the test client certificate.

    :return: The path to the certificate.
    :rytpe: string
    """
    return _get_file_from_helpers("certificates", "TestClientCert.pem")


def _get_path_to_private_key() -> str:
    """Gets the path to the private key certificate.

    :return: The path to the certificate.
    :rytpe: string
    """
    return _get_file_from_helpers("certificates", "TestClientPrivateKey.pem")


def _path_to_root_cert() -> str:
    """Gets the path to the root certificate.

    :return: The path to the certificate.
    :rytpe: string
    """
    return _get_file_from_helpers("certificates", "TestRootCert.pem")


def _get_file_from_helpers(folder: str, file: str) -> str:
    """Gets a file location in a folder.

    :param folder: The name of the folder to search in.
    :type folder: string
    :param file: The name of the folder to search in.
    :type file: string

    :return: The path to the file.
    :rytpe: string
    """
    return os.path.join(
        os.path.dirname(os.path.realpath(__file__)), f"{folder}/{file}"
    )


def validate_json(data: dict, expected_schema: dict) -> bool:
    """Validates that a response meets the expected schema format

    :param data: The response to validate.
    :type data: dict
    :param file: The schema to validate against.
    :type file: dict

    :return: True if the validation succeeds, False otherwise
    :rtype: bool
    """
    try:
        validate(instance=data, schema=expected_schema)
    except jsonschema.ValidationError:
        return False
    return True


def get_image_components() -> list:
    """Gets all images on a device.

    :return: The list of images.
    :rtype: list[dict]
    """
    response = image.get_images()

    return response["components"]


def clean_images():
    """Cleans the application images that do not have the name gdb server."""

    timeout = 5
    current_time = 0
    install_response = ""
    while current_time < timeout:
        try:
            install_response = sideload.install_images()
            break
        except requests.exceptions.ConnectionError:
            time.sleep(0.5)
            current_time += 0.5

    if install_response != {}:
        raise requests.exceptions.ConnectionError("Cannot clean images, installing images failed.")

    components = [app for app in get_image_components() if app["image_type"] == 10]

    if len(components) != 0:
        _delete_images_without_name(components, "gdbserver")


def clean_wifi_networks():
    """Removes all added wifi networks."""
    response = wifi.get_all_wifi_networks()

    for net in response["values"]:
        wifi.remove_configured_wifi_network(net["id"])


def _delete_images_without_name(components: list, name: str):
    """Cleans all application images except those that have the specified name

    :param components: The list of applications.
    :type components: list[dict]
    :param name: The name of the applications to keep.
    :type name: str
    """

    for component in components:
        component_name = component["name"]
        if component_name != name:
            uid = component["uid"]

            sideload.delete_image(uid)


def clean_certificates():
    """Removes the added certificates."""
    response = certificate.get_all_certificates()
    certs = response["identifiers"]

    for cert in certs:
        certificate.remove_certificate(cert)


# Path to AzureSphereBlinkTest.imagepackage sample application.<
path_to_blink_image = _get_blink_path()
# Path to MutableStorage.imagepackage sample application.
path_to_mutable_storage = _get_path_to_mutable_storage()

# Path to test client certificate pem file.
path_to_client_cert = _get_test_client_cert()
# Path to test client private key pem file.
path_to_client_private_key = _get_path_to_private_key()
# Path to test root certificate pem file.
path_to_root_cert = _path_to_root_cert()

# A randomly generated component id.
random_uuid = "6770fc06-e7db-11ec-8fea-0242ac120002"
# Component id of the AzureSphereBlinkTest.imagepackage sample application.
blink_component_id = "2f074b0f-d99d-4692-82d9-ef93a7d6463c"
# Component id of the MutableStorage.imagepackage sample application.
mutable_storage_id = "ae4714aa-03aa-492b-9663-962f966a9cc3"
