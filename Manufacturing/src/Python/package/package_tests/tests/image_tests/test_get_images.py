# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import image, sideload
from tests.helpers import utils


def test__get_images__returns_correct_format(fix_clean_images):
    """Tests if getting images returns all images in correct format."""
    top_level_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {'is_ota_update_in_progress': {'type': 'boolean'}, 'has_staged_updates': {'type': 'boolean'}, 'restart_required': {'type': 'boolean'}, 'components': {'type': 'array'}}
    }

    components_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {'uid': {'type': 'string'}, 'image_type': {'type': 'integer'}, 'is_update_staged': {'type': 'boolean'}, 'does_image_type_require_restart': {'type': 'boolean'}, 'images': {'type': 'array'}, 'name': {'type': 'string'}}
    }

    image_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {'uid': {'type': 'string'}, 'length_in_bytes': {'type': 'integer'}, 'uncompressed_length_in_bytes': {'type': 'integer'}, 'replica_type': {'type': 'integer'}}
    }

    response = image.get_images()

    assert utils.validate_json(response, top_level_schema)

    for component in response["components"]:
        assert utils.validate_json(component, components_schema)

        for img in component["images"]:
            assert utils.validate_json(img, image_schema)


def test__get_images__add_image_reflected_in_get_images(fix_clean_images):
    """Tests if adding an image is reflected in a get images call."""
    response = image.get_images()

    assert not any(
        elem["uid"] == utils.blink_component_id for elem in response["components"])

    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    end_response = image.get_images()
    assert any(
        elem["uid"] == utils.blink_component_id for elem in end_response["components"])


def test__get_images__add_then_delete_shows_removed_image(fix_clean_images):
    """Tests if adding and then deleting an image shows no change in get images."""
    sideload.stage_image(utils.path_to_blink_image)
    sideload.install_images()

    response = image.get_images()
    assert any(
        elem["uid"] == utils.blink_component_id for elem in response["components"])

    sideload.delete_image(utils.blink_component_id)

    end_response = image.get_images()

    assert not any(
        elem["uid"] == utils.blink_component_id for elem in end_response["components"])
