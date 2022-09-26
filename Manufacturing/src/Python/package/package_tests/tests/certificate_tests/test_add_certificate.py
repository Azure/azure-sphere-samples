# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import certificate, exceptions
from tests.helpers import utils


def test__add_certificate__null_certificate_location_throws_validation_error():
    """Tests if adding a certificate with a null certificate location throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, certificate path is null or empty."):
        certificate.add_certificate(
            "Ignore", None, "Ignore", "Ignore", "Ignore")


def test__add_certificate__empty_certificate_location_throws_validation_error():
    """Tests if adding a certificate with an empty string as the certificate location throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, certificate path is null or empty."):
        certificate.add_certificate("Ignore", "", "Ignore", "Ignore", "Ignore")


def test__add_certificate__invalid_certificate_throws_validation_error():
    """Tests if adding a certificate with an invalid file location throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, certificate path is invalid."):
        certificate.add_certificate(
            "Ignore", "InvalidCertificateLocation", "client")


def test__add_certificate__null_certificate_id_throws_validation_error():
    """Tests if adding a certificate with a null certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, certificate ID is null "):
        certificate.add_certificate(None, "Ignore", "Ignore")


def test__add_certificate__empty_certificate_id_throws_validation_error():
    """Tests if adding a certificate with an empty certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, certificate ID is null "):
        certificate.add_certificate("", "Ignore", "Ignore")


def test__add_certificate__null_cert_type_throws_validation_error():
    """Tests if adding a certificate with a null certificate type throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, cert type is invalid"):
        certificate.add_certificate("Ignore", "Ignore", None)


def test__add_certificate__empty_cert_type_throws_validation_error():
    """Tests if adding a certificate with an empty certificate type throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, cert type is invalid"):
        certificate.add_certificate("Ignore", "Ignore", "")


def test__add_certificate__invalid_cert_type_throws_validation_error():
    """Tests if adding a certificate with an invalid certificate type throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, cert type is invalid"):
        certificate.add_certificate("Ignore", "Ignore", "NotAValidCertType")


def test__add_certificate__valid_cert_type_returns_ok(fix_clean_certificates):
    """Tests if adding a certificate with valid certificate types returns empty json responses."""
    root_response = certificate.add_certificate(
        "root_id", utils.path_to_root_cert, "rootca")

    assert {} == root_response

    client_response = certificate.add_certificate(
        "client_id", utils.path_to_client_cert, "rootca", utils.path_to_client_private_key, "password")

    assert {} == client_response


def test__add_certificate__client_null_private_key_throws_validation_error():
    """Tests if adding a client certificate with a null private key location throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, private key path is invalid."):
        certificate.add_certificate(
            "client_id", utils.path_to_client_cert, "client", None, "password")


def test__add_certificate__client_empty_private_key_throws_validation_error():
    """Tests if adding a client certificate with an empty string as a private key location throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, private key path is invalid."):
        certificate.add_certificate(
            "client_id", utils.path_to_client_cert, "client", "", "password")


def test__add_certificate__client_null_password_throws_validation_error():
    """Tests if adding a client certificate with a null password throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, invalid password"):
        certificate.add_certificate("client_id", utils.path_to_client_cert,
                                    "client", utils.path_to_client_private_key, None)


def test__add_certificate__client_empty_password_throws_validation_error():
    """Tests if adding a client certificate with an empty password throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot add certificate, invalid password"):
        certificate.add_certificate("client_id", utils.path_to_client_cert,
                                    "client", utils.path_to_client_private_key, "")
