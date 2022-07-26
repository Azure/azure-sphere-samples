# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import device


def test__get_device_rest_api_version__returns_correct():
    """Tests if getting the device rest api version returns the expected version in the expected format."""
    response = device.get_device_rest_api_version()

    assert {"REST-API-Version": "4.4.0"} == response
