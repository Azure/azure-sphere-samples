# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import time

import requests
from microsoft_azure_sphere_deviceapi import device


def test__restart_device__makes_uptime_restart():
    """Tests if restarting the device restarts the uptime."""
    assert _wait_till_up()

    time.sleep(5)
    start_response = device.get_device_status()["uptime"]

    device.restart_device()
    assert _wait_till_up()

    end_response = device.get_device_status()["uptime"]

    assert end_response < start_response


def test__restart_device__returns_correct_format():
    """Tests if restarting the device returns that it is restarting the system."""
    assert _wait_till_up()
    response = device.restart_device()

    assert response == {'restartingSystem': True}
    assert _wait_till_up


def _wait_till_up() -> bool:
    """Helper function that waits till the device has restarted or reaches a timeout then returns.

    :return: True if device has responded in time, False otherwise.
    :rtype: bool
    """
    max_wait_time = 5000
    current_wait_time = 0

    while current_wait_time < max_wait_time:
        try:
            device.get_device_status()
            return True
        except requests.exceptions.ConnectionError:
            time.sleep(0.25)
            current_wait_time += 250

    return False
