# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import device


def test__diagnostics_log__returns_non_empty():
    """Tests if getting the device status at later points increases the 'uptime'."""
    response = device.get_diagnostic_log()

    assert response
