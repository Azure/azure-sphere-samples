# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import app, exceptions, sideload
from tests.helpers import utils


def test__get_app_quota__null_component_throws_validation_error():
    """Tests if getting the app quota with a null component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get the app quota"):
        app.get_app_quota(None)


def test__get_app_quota__empty_component_throws_validation_error():
    """Tests if getting the app quota with an empty component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get the app quota"):
        app.get_app_quota("")


def test__get_app_quota__non_uuid_component_throws_validation_error():
    """Tests if getting the app quota with an invalid component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError, match="ERROR: Cannot get the app quota"):
        app.get_app_quota("1234")


def test__get_app_quota__valid_non_existent_component_throws_validation_error():
    """Tests if getting the app quota with a valid but non existent component id throws a device error"""
    with pytest.raises(exceptions.DeviceError, match="Application is not present"):
        app.get_app_quota(utils.random_uuid)


def test__get_app_quota__valid_existing_component_returns_ok(fix_clean_images):
    """Tests if getting the app quota with an existing component id returns the expected values for usage and limit."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    quotaResponse = app.get_app_quota(utils.blink_component_id)

    assert quotaResponse == {'UsageKB': 0, 'LimitKB': 0}


def test__get_app_quota__deleted_app_quota_throws_device_error(fix_clean_images):
    """Tests if getting the app quota with the component id of a newly deleted app throws a device error."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    sideload.delete_image(utils.blink_component_id)

    with pytest.raises(exceptions.DeviceError, match="Application is not present"):
        app.get_app_quota(utils.blink_component_id)


def test__get_app_quota__stage_mutable_storage_app__increases_values(fix_clean_images):
    """Tests if deploying an image with mutable storage modifies the limit."""
    sideload.stage_image(utils.path_to_mutable_storage)
    sideload.install_images()

    blink_response = app.get_app_quota(utils.mutable_storage_id)

    assert {'UsageKB': 0, 'LimitKB': 8} == blink_response
