# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import app
from tests.helpers import utils


def test__memory_statistics__valid_response_format():
    """Tests if getting the memory statistics returns in the expected format."""

    response = app.get_memory_statistics()
    assert _validate_memory_stats_format(response)


def _validate_memory_stats_format(response):
    """Helper function to validate the format of a response matches the expected format.

    :param response: The response to validate.
    :type response: dict

    :return: True if the response is successfully validated against the schema, false otherwise.
    :rtype: bool
    """
    inner_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'currentMemoryUsageInBytes': {'type': 'integer'},
                'peakUserModeMemoryUsageInBytes': {'type': 'integer'},
                'userModeMemoryUsageInBytes': {'type': 'integer'}
        }
    }
    return "memoryStats" in response and utils.validate_json(response['memoryStats'], inner_schema)
