# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from azuresphere_device_api import exceptions, sideload
from tests.helpers import utils


def test__delete_image__null_component_throws_validation_error():
    """Tests if deleting an image with a null component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        sideload.delete_image(None)
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot delete image, invalid component ID.'


def test__delete_image__empty_component_throws_validation_error():
    """Tests if deleting an image with an empty string component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        sideload.delete_image("")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot delete image, invalid component ID.'


def test__delete_image__non_uuid_component_throws_validation_error():
    """Tests if deleting an image with a non existant component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        sideload.delete_image("InvalidUuid")
    assert error.exconly() == 'azuresphere_device_api.exceptions.ValidationError: ERROR: Cannot delete image, invalid component ID.'


def test__delete_image__valid_non_existent_uuid_component_returns_empty_json_response():
    """Tests if deleting a valid component id returns an empty json response."""
    response = sideload.delete_image(utils.random_uuid)

    assert response == {}


def test__delete_image__delete_existing_image_returns_returns_empty_json_response(fix_clean_images):
    """Tests if deleting an existing image's component id returns an empty json response."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    response = sideload.delete_image(utils.blink_component_id)

    assert response == {}


def test__delete_image__delete_image_reflected_in_get_images(fix_clean_images):
    """Tests if deleting an image is reflected in the current devices image components."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    images = utils.get_image_components()
    running_apps_start = len(
        [item for item in images if item["image_type"] == 10])
    assert len([item for item in images if item["uid"]
               == utils.blink_component_id]) != 0

    sideload.delete_image(utils.blink_component_id)
    end_images = utils.get_image_components()
    assert len([item for item in end_images if item["uid"]
               == utils.blink_component_id]) == 0

    running_apps_end = len(
        [item for item in end_images if item["image_type"] == 10])
    assert(running_apps_start > running_apps_end)
