# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import time

from azuresphere_device_api import device


def test__clear_error_report_data__returns_empty():
    """Tests if clearing the error report data returns an empty json response."""
    response = device.clear_error_report_data()

    assert {} == response


def test__clear_error_report_data__clears_error_report_data():
    """Tests if clearing the error report data clears the error report data."""
    response = device.clear_error_report_data()

    assert {} == response

    max_milliseconds = 1000
    elapsed_milliseconds = 0

    data_length = _get_data_length(device.get_error_report_data())

    while data_length != 0 and elapsed_milliseconds < max_milliseconds:
        time.sleep(0.1)
        elapsed_milliseconds += 100
        data_length = _get_data_length(device.get_error_report_data())

    assert data_length == 0


def _get_data_length(response: bytes):
    """Helper class that gets the data length bytes of the error response data.

    :param response: The error response data.
    :type response: bytes

    :return: THe data length field
    :rtype: int
    """
    return response[3] << 8 | response[4]
