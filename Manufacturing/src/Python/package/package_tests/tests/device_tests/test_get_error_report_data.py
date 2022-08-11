# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import device


def test__get_error_report_data__returns_non_empty():
    """Tests if getting the error report data returns a non null or empty response."""
    response = device.get_error_report_data()

    assert response
