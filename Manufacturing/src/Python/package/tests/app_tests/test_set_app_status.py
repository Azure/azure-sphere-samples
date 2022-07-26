# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import pytest
from microsoft_azure_sphere_deviceapi import app, exceptions, sideload
from tests.helpers import utils


def test__set_app_status__null_component_id_throws_validation_error():
    """Tests if setting the app status with a null component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status(None, "")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid component ID.'


def test__set_app_status__empty_component_id_throws_validation_error():
    """Tests if setting the app status with an empty component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status("", "")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid component ID.'


def test__set_app_status__not_uuid_format_throws_validation_error():
    """Tests if setting the app status with a non uuid format component id throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status("1234", "")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid component ID.'


def test__set_app_status__null_trigger_throws_validation_error():
    """Tests if setting the app status with a null trigger throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status(utils.random_uuid, None)
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid trigger.'


def test__set_app_status__empty_trigger_throws_validation_error():
    """Tests if setting the app status with an empty trigger throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status(utils.random_uuid, "")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid trigger.'


def test__set_app_status__invalid_trigger_throws_validation_error():
    """Tests if setting the app status with a non-valid trigger throws a validation error."""
    with pytest.raises(exceptions.ValidationError) as error:
        app.set_app_status(utils.random_uuid, "InvalidTrigger!")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.ValidationError: ERROR: Cannot set the app status, invalid trigger.'


def test__set_app_status__valid_triggers_return_correct_states(fix_clean_images):
    """Tests if setting the app status with valid triggers returns corresponding states."""
    response = sideload.stage_image(utils.path_to_blink_image)
    assert response == {}

    sideload.install_images()

    debug_response_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {'state': {'type': 'string'},
                       'gdbPort': {'type': 'integer'},
                       'outPort': {'type': 'integer'}
                       }
    }

    start_state = ('start', {'state': 'running'})
    debug_state_key = "startDebug"
    stop_state = ('stop', {'state': 'stopped'})

    start_response = app.set_app_status(utils.blink_component_id, start_state[0])
    assert start_response == start_state[1]

    debug_response = app.set_app_status(utils.blink_component_id, debug_state_key)
    assert utils.validate_json(debug_response, debug_response_schema)

    stop_response = app.set_app_status(utils.blink_component_id, stop_state[0])
    assert stop_response == stop_state[1]


def test__set_app_status__random_component_id_throws_device_error():
    """Tests if setting the app status with a random uuid component id that doesn't exist throws a device error."""
    with pytest.raises(exceptions.DeviceError) as error:
        app.set_app_status(utils.random_uuid, "start")
    assert error.exconly() == 'microsoft_azure_sphere_deviceapi.exceptions.DeviceError: ERROR: The device could not perform this request due to the resource being in a conflicting state. Application is not present'
