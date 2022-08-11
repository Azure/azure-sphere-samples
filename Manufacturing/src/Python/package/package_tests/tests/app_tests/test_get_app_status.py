# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import app, exceptions, sideload
from tests.helpers import utils


def test__get_app_status__null_component_throws_validation_error():
    """Tests if getting the app status with a null component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.get_app_status(None)
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot get the app status, invalid component ID.'


def test__get_app_status__empty_component_throws_validation_error():
    """Tests if getting the app status with an empty component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.get_app_status("")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot get the app status, invalid component ID.'


def test__get_app_status__non_uuid_component_throws_validation_error():
    """Tests if getting the app statis with a non uuid format component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.get_app_status("1234")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot get the app status, invalid component ID.'


def test__get_app_status__valid_non_existent_component_returns_not_present(fix_clean_images):
    """Tests if getting the app statis when component id is valid uuid but doesnt exist returns a not present response."""
    response = app.get_app_status(utils.random_uuid)
    assert response == {'state': 'notPresent'}


def test__get_app_status__valid_existing_component_returns_valid_state(fix_clean_images):
    """Tests if getting the app statis when component id exists returns is running."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    quotaResponse = app.get_app_status(utils.blink_component_id)
    assert quotaResponse == {'state': 'running'}


def test__get_app_status__newly_staged_component_not_running(fix_clean_images):
    """Tests if getting the app status of a newlyt staged component returns not present."""
    sideload.stage_image(utils.path_to_blink_image)

    response = app.get_app_status(utils.blink_component_id)
    assert response == {'state': 'notPresent'}


def test__get_app_status__newly_installed_component_is_running(fix_clean_images):
    """Tests if getting the app status of a newly deleted component returns not present."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    response = app.get_app_status(utils.blink_component_id)
    assert response == {'state': 'running'}


def test__get_app_status__newly_deleted_component_is_running(fix_clean_images):
    """Tests if getting the app status of a newly installed component returns is running."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()
    sideload.delete_image(utils.blink_component_id)

    response = app.get_app_status(utils.blink_component_id)
    assert response == {'state': 'notPresent'}


def test__get_app_status__installing_image_changes_state(fix_clean_images):
    """Tests if installing an image is reflected in getting the app status."""
    sideload.stage_image(utils.path_to_blink_image)

    start_response = app.get_app_status(utils.blink_component_id)

    assert start_response == {'state': 'notPresent'}

    sideload.install_images()

    end_response = app.get_app_status(utils.blink_component_id)
    assert end_response == {'state': 'running'}
