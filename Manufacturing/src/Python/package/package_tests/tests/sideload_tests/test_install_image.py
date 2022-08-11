# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import app, sideload
from tests.helpers import utils


def test__install_images__returns_empty_json_response(fix_clean_images):
    """Tests if installing images returns an empty json response."""
    response = sideload.install_images()

    assert {} == response


def test__install_images__installing_component_returns_expected_state(fix_clean_images):
    """Tests if a newly installed image is running."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'running'} == response


def test__install_images__installing_image_changes_state(fix_clean_images):
    """Tests if installing images changes the state of the image."""
    sideload.stage_image(utils.path_to_blink_image)

    start_response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'notPresent'} == start_response

    sideload.install_images()

    install_response = app.get_app_status(utils.blink_component_id)
    assert {'state': 'running'} == install_response
