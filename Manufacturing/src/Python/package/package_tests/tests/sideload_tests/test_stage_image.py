# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import app, exceptions, sideload
from tests.helpers import utils


def test__stage_image__null_image_location_throws_validation_error():
    """Tests if staging an image with a null image location throws validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot stage image, image path is null or empty"):
        sideload.stage_image(None)


def test__stage_image__empty_image_location_throws_validation_error():
    """Tests if staging an image with an empty string location throws validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot stage image, image path is null or empty"):
        sideload.stage_image("")


def test__stage_image__non_file_location_throws_validation_error():
    """Tests if staging an image with a non file location throws validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot stage image, image path is null or empty"):
        sideload.stage_image("NotAFileLocation")


def test__stage_image__file_location_returns_bad_request(fix_clean_images):
    """Tests if staging an image with a valid file path returns an empty json response."""
    response = sideload.stage_image(utils.path_to_blink_image)

    assert response == {}


def test__stage_image__stage_image_doesnt_deploy_image(fix_clean_images):
    """Tests if staging an image doesnt deploy the image."""
    start_response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'notPresent'} == start_response

    sideload.stage_image(utils.path_to_blink_image)

    end_response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'notPresent'} == end_response


def test__stage_image__staging_image_enables_installation_state_changes_to_running(fix_clean_images):
    """Tests if deploying an image changes its state to running."""
    start_response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'notPresent'} == start_response

    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    end_response = app.get_app_status(utils.blink_component_id)

    assert {'state': 'running'} == end_response
