# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import certificate, exceptions
from tests.helpers import utils


def test__get_certificate__null_certificate_id_throws_validation_error():
    """Getting a certificate with a null certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get certificate, certificate ID is null or empty"):
        certificate.get_certificate(None)


def test__get_certificate__empty_certificate_id_throws_validation_error():
    """Tests if getting a certificate with an empty certificate id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get certificate, certificate ID is null or empty"):
        certificate.get_certificate("")


def test__get_certificate__valid_certificate_id_returns_expected_cert(fix_clean_certificates):
    """Tests if getting an existing certificate returns the certificate."""
    certificate.add_certificate("test_cert", utils.path_to_root_cert, "rootca")
    response = certificate.get_certificate("test_cert")

    assert {'id': 'test_cert', 'notBefore': '2022-06-17T09:12:46', 'notAfter': '2023-06-17T09:32:46',
            'subjectName': '/CN=TestRootCert', 'issuerName': '/CN=TestRootCert'} == response


def test__get_certificate__delete_certificate_then_get_throws_device_error(fix_clean_certificates):
    """Tests if getting a certificate after it has been deleted throws a device error."""
    certificate.add_certificate("test_cert", utils.path_to_root_cert, "rootca")
    certificate.remove_certificate("test_cert")

    with pytest.raises(exceptions.DeviceError, match="No certificate with that identifier could be found on the device."):
        certificate.get_certificate("test_cert")
