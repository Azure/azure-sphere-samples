# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from os import path
from typing import Literal

from azuresphere_device_api import utils
from azuresphere_device_api.exceptions import ValidationError

__all__ = ['add_certificate', 'get_all_certificates',
           'get_certificate', 'get_certificate_space', 'remove_certificate']


def add_certificate(
    certificate_id: str,
    certificate_path: str,
    cert_type: Literal["client", "rootca"],
    private_key_path="",
    password="",
) -> dict:
    """Makes a "POST" request to add a certificate to the attached devices certificate store.

    :param certificate_id: The ID of the certificate.
    :type certificate_id: string
    :param certificate_path: The absolute path to a public key certificate .pem file.
    :type certificate_path: string
    :param cert_type: The type of certificate to add.
    :type cert_type: string
    :accepted-values cert_type: client, rootca
    :param private_key_path: The absolute path to a client private key .pem file. Required when  
                             adding a certificate of type 'client'.
    :type private_key_path: string
    :param password: The password to decrypt the private key. Required when adding a
                     client private key that is encrypted.
    :type password: string

    :return: An empty response on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not certificate_id:
        raise ValidationError(
            "ERROR: Cannot add certificate, certificate ID is null or empty.")

    if not certificate_path:
        raise ValidationError(
            "ERROR: Cannot add certificate, certificate path is null or empty.")

    valid_cert_types = ["client", "rootca"]

    if not cert_type or cert_type not in valid_cert_types:
        raise ValidationError(
            "ERROR: Cannot add certificate, cert type is invalid.")

    if not path.exists(certificate_path) or not path.isfile(certificate_path):
        raise ValidationError(
            "ERROR: Cannot add certificate, certificate path is invalid.")

    cert = open(certificate_path, "r")

    response = None
    if cert_type == "client":
        if not private_key_path:
            raise ValidationError(
                "ERROR: Cannot add certificate, private key path is invalid.")

        if not password:
            raise ValidationError(
                "ERROR: Cannot add certificate, invalid password.")

        private_key_file = open(private_key_path, "r")
        response = utils.post_request(
            f"certstore/certs/{certificate_id}",
            {
                "certType": cert_type,
                "publicCert": cert.read(),
                "privateKey": private_key_file.read(),
                "password": password,
            },
        )
        private_key_file.close()
    else:
        response = utils.post_request(
            f"certstore/certs/{certificate_id}",
            {"certType": cert_type, "publicCert": cert.read()},
        )
    cert.close()
    return response


def get_all_certificates() -> dict:
    """List certificates in the attached device's certificate store.

    :return: The identifiers of the attached certificates on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("certstore/certs")


def get_certificate(certificate_id: str) -> dict:
    """Makes a REST "GET" request to get information about a specific certificate.

    :param certificate_id: The ID of the certificate.
    :type certificate_id: string

    :return: Details of the certificate on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not certificate_id:
        raise ValidationError(
            "ERROR: Cannot get certificate, certificate ID is null or empty.")

    return utils.get_request(f"certstore/certs/{certificate_id}")


def get_certificate_space() -> dict:
    """Makes a REST "GET" request to show the available free space in the attached device's
    certificate store.

    :return:The amount of free space for certificates on success. An exception will be thrown on error.
    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    return utils.get_request("certstore/space")


def remove_certificate(certificate_id: str) -> dict:
    """Makes a "DELETE" request to delete a certificate in the attached device's certificate store.

    :param certificate_id: The ID of the certificate.
    :type certificate_id: string

    :return: An empty response on success. An exception will be thrown on error.

    :rtype: dict[str, str]
    :raises: requests.exceptions
    :raises: AzureSphereDeviceApiException
    """
    if not certificate_id:
        raise ValidationError(
            "ERROR: Cannot get certificate, certificate ID is null or empty.")

    return utils.delete_request(f"certstore/certs/{certificate_id}")
