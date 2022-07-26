# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from microsoft_azure_sphere_deviceapi import network
from tests.helpers import utils


def test__firewall_rulesets__returns_correct_format():
    """Tests if getting the firewall ruleset returns rules in the correct format."""
    response = network.get_network_firewall_ruleset()

    top_level_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'rulesets': {'type': 'array'}
        }
    }

    ruleset_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'hook': {'type': 'string'},
            'isValid': {'type': 'boolean'},
            'rules': {'type': 'array'}
        }
    }

    rule_schema = {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            'sourceIP': {'type': 'string'},
            'sourceMask': {'type': 'string'},
            'destinationIP': {'type': 'string'},
            'destinationMask': {'type': 'string'},
            'uid': {'type': 'integer'},
            'action': {'type': 'string'},
            'interfaceInName': {'type': 'string'},
            'interfaceOutName': {'type': 'string'},
            'state': {'type': 'string'},
            'tcpMask': {'type': 'array'},
            'tcpCmp': {'type': 'array'},
            'tcpInv': {'type': 'boolean'},
            'protocol': {'type': 'string'},
            'sourcePortRange': {'min': {'type': 'integer'},
                                'max': {'type': 'integer'}},
            'destinationPortRange': {'min': {'type': 'integer'},
                                     'max': {'type': 'integer'}},
            'packets': {'type': 'integer'},
            'bytes': {'type': 'integer'}
        }
    }
    assert utils.validate_json(response, top_level_schema)

    for ruleset in response["rulesets"]:
        assert utils.validate_json(response, ruleset_schema)
        for rule in ruleset["rules"]:
            assert utils.validate_json(rule, rule_schema)
