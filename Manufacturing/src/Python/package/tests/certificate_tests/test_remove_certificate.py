# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from microsoft_azure_sphere_deviceapi import certificate, exceptions
from tests.helpers import utils


def test__remove_certificate__null_cert_id_throws_validation_error():
    """Tests if removing a certificate with a null certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        certificate.remove_certificate(None)
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot get certificate, certificate ID is null or empty.'


def test__remove_certificate__empty_cert_id_throws_validation_error():
    """Tests if removing a certificate with an empty certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        certificate.remove_certificate("")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot get certificate, certificate ID is null or empty.'


def test__remove_certificate__remove_existing_certificate_returns_empty_json_response(fix_clean_certificates):
    """Tests if removing an existing certificate returns an empty json response."""
    certificate.add_certificate("TestCert", utils.path_to_root_cert, "rootca")
    response = certificate.remove_certificate("TestCert")

    assert {} == response


def test__remove_certificate__remove_non_existing_certificate_returns_not_found(fix_clean_certificates):
    """Tests if removing a non existing certificate throws a device error."""
    with pytest.raises(exceptions.DeviceError) as error:
        certificate.remove_certificate("TestCert")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.DeviceError: ERROR: This resource is unavailable on this device. No certificate with that identifier could be found on the device.'


def test__remove_certificate__removing_removed_certificate_returns_not_found(fix_clean_certificates):
    """Tests if removing a removed certificate throws a device error."""
    certificate.add_certificate("TestCert", utils.path_to_root_cert, "rootca")
    certificate.remove_certificate("TestCert")

    with pytest.raises(exceptions.DeviceError) as error:
        certificate.remove_certificate("TestCert")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.DeviceError: ERROR: This resource is unavailable on this device. No certificate with that identifier could be found on the device.'
