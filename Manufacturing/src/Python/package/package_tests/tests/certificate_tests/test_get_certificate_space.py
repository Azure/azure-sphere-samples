# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import certificate
from tests.helpers import utils


def test__get_certificate_space__returns_list(fix_clean_certificates):
    """Tests if getting the certificate space returns a response in the expected format."""
    response = certificate.get_certificate_space()

    assert {'AvailableSpace': "24514"} == response


def test__get_certificate_space_adding_certificate_reduces_space__returns_list(fix_clean_certificates):
    """Tests if getting the certificate space after adding a certificate, decreases the certificate space."""
    response = certificate.get_certificate_space()
    start_space = response["AvailableSpace"]

    certificate.add_certificate("root_id", utils.path_to_root_cert, "rootca")

    end_response = certificate.get_certificate_space()
    end_space = end_response["AvailableSpace"]
    assert start_space > end_space


def test__get_certificate_space_deleting_certificate_reduces_space__returns_list(fix_clean_certificates):
    """Tests if getting the certificate space after removing a certificate increases the certificate space."""
    certificate.add_certificate("root_id", utils.path_to_root_cert, "rootca")

    response = certificate.get_certificate_space()
    start_space = response["AvailableSpace"]

    certificate.remove_certificate("root_id")

    end_response = certificate.get_certificate_space()
    end_space = end_response["AvailableSpace"]
    assert start_space < end_space
