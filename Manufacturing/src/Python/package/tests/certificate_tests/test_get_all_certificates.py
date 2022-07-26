# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import certificate
from tests.helpers import utils


def test__get_all_certificates__returns_empty_list(fix_clean_certificates):
    """Tests if getting all certificates without adding a certificate returns an empty identifiers list."""
    response = certificate.get_all_certificates()

    assert {'identifiers': []} == response


def test__get_all_certificates__adding_cert_adds_cert(fix_clean_certificates):
    """Tests if adding a certificate is reflected in calls to get all certificates."""
    response = certificate.get_all_certificates()

    assert {'identifiers': []} == response

    certificate.add_certificate("test_cert", utils.path_to_root_cert, "rootca")

    end_response = certificate.get_all_certificates()

    assert {'identifiers': ["test_cert"]} == end_response


def test__get_all_certificates__deleting_cert_deletes_cert(fix_clean_certificates):
    """Tests if deleting a certificate is reflected in calls to get all certificates."""
    certificate.add_certificate("test_cert", utils.path_to_root_cert, "rootca")

    start_response = certificate.get_all_certificates()

    assert {'identifiers': ["test_cert"]} == start_response

    certificate.remove_certificate("test_cert")

    response = certificate.get_all_certificates()

    assert {'identifiers': []} == response
