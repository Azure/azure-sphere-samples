# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import certificate, exceptions
from tests.helpers import utils


def test__remove_certificate__null_cert_id_throws_validation_error():
    """Tests if removing a certificate with a null certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get certificate, certificate ID is null or empty"):
        certificate.remove_certificate(None)


def test__remove_certificate__empty_cert_id_throws_validation_error():
    """Tests if removing a certificate with an empty certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get certificate, certificate ID is null or empty"):
        certificate.remove_certificate("")


def test__remove_certificate__remove_existing_certificate_returns_empty_json_response(fix_clean_certificates):
    """Tests if removing an existing certificate returns an empty json response."""
    certificate.add_certificate("TestCert", utils.path_to_root_cert, "rootca")
    response = certificate.remove_certificate("TestCert")

    assert {} == response


def test__remove_certificate__remove_non_existing_certificate_returns_not_found(fix_clean_certificates):
    """Tests if removing a non existing certificate throws a device error."""
    with pytest.raises(exceptions.DeviceError, match="No certificate with that identifier could be found on the device"):
        certificate.remove_certificate("TestCert")


def test__remove_certificate__removing_removed_certificate_returns_not_found(fix_clean_certificates):
    """Tests if removing a removed certificate throws a device error."""
    certificate.add_certificate("TestCert", utils.path_to_root_cert, "rootca")
    certificate.remove_certificate("TestCert")

    with pytest.raises(exceptions.DeviceError, match="No certificate with that identifier could be found on the device"):
        certificate.remove_certificate("TestCert")
