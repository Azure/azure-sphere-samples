#!/usr/bin/env python3

'''
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

device_ready.py

This sample script demonstrates various automated ready-to-ship checks. Please note that some checks
can only be performed before the device manufacturing state is finalised.
'''

import argparse
import json
import logging
import os
import sys

from microsoft_azure_sphere_deviceapi import image, devices, capabilities, wifi, manufacturing
from microsoft_azure_sphere_deviceapi.exceptions import AzureSphereDeviceApiException

class DeviceReadyChecker:
    """
    Class which performs device ready checks
    """

    def __init__(self, logger=logging.getLogger(__name__)):
        """
        Create a DeviceReadyCheck class
        Parameter:
        azsphere_cli_helper: Instance of an AzsphereCliHelper object for communicating with the device
        logger: Instance of a logger for logging messages
        """
        # self.cli_helper = azsphere_cli_helper
        self.logger = logger

    def check_no_capabilities(self):
        """
        Ensure device contains no capabilities
        Returns: True if device has no capabilites
        """
        self.logger.info('Checking capabilities...')

        caps = capabilities.get_device_capabilities()
        if len(caps['device_capabilities']) != 0:
            self.logger.info('FAIL: Device contains capabilities:')
            s = [str(i) for i in caps['device_capabilities']]
            self.logger.info(','.join(s))
            return False

        self.logger.info('PASS: No capabilties on device')
        return True

    def check_no_wifi_networks(self):
        """
        Ensure device has no configured wifi networks
        Returns: True if device has no APs
        """
        self.logger.info('Checking wifi networks...')

        networks = wifi.get_all_wifi_networks()
        if len(networks['values']) != 0:
            self.logger.info('FAIL: Device contains configured networks:')
            for network in networks['values']:
                self.logger.info('{0}: {1}'.format(network["id"], network["ssid"]))
            return False

        self.logger.info('PASS: Device has no wifi networks configured')
        return True

    def get_manufacturing_state(self):
        """
        Returns: The current manufacturing state.
        """
        return manufacturing.get_device_manufacturing_state()['manufacturingState']

    def check_manufacturing_state(self, expected_state, device_state):
        """
        Ensure device is in given manufacturing state
        Returns: True if device is in given state
        """
        self.logger.info(f'Checking device is in manufacturing state {expected_state}...')

        if device_state != expected_state:
            self.logger.info(
                f"FAIL: Device manufacturing state is {device_state}, expected state {expected_state}")
            return False

        self.logger.info(f"PASS: Device manufacturing state is {device_state}")
        return True

    def check_os_version(self, expected_os_versions, image_ids_json_file):
        """
        Ensure device has the correct OS version
        Parameters:
        expected_os_versions: list of system software release names
        image_ids_json_file: file containing a mapping of image ids to OS versions
        """
        try:
            with open(image_ids_json_file, "r") as file:
                image_ids_json = json.load(file)
        except Exception as e:
            raise Exception(
                f"Error occurred parsing image id json file '{image_ids_json_file}': {e}")

        self.logger.info('Checking OS version...')

        success = True
        os_version = "unknown"

        all_components = image.get_images()['components']

        # Warn if expected_os_versions don't appear in the supplied json file
        list_of_possible_os_versions = list(map(lambda x: x["name"], image_ids_json["versions"]))
        invalid_requested_versions = []
        for version in expected_os_versions:
            if version not in list_of_possible_os_versions:
                invalid_requested_versions.append(version)

        if len(invalid_requested_versions) > 1:
            self.logger.warning("Requested OS versions '{}' are not recognised as possible OS"
                                " versions; options are: {}".format(", ".join(invalid_requested_versions),
                                                                    list_of_possible_os_versions))
        elif len(invalid_requested_versions) > 0:
            self.logger.warning("Requested OS version '{}' is not recognised as a possible OS"
                                " version; options are: {}".format(invalid_requested_versions[0],
                                                                   list_of_possible_os_versions))

        # Format the list for easy matching
        device_components = list(
            map(lambda component: {"iid": component['images'][0]['uid'], "cid": component['uid']}, all_components))

        # Compare with all possible OS versions until we find a matching one
        for os_version_info in image_ids_json["versions"]:
            matches = True
            for img in os_version_info["images"]:
                if img not in device_components:
                    matches = False
                    break
            if matches:
                os_version = os_version_info["name"]
                break

        if os_version not in expected_os_versions:
            success = False

        # Make sure no special system applications (like RF test server) are present
        special_components = [
            '508eddf2-b8c3-4a99-93d9-a045d9a060eb',  # rftest_server
        ]

        for component in special_components:
            if component in [c['cid'] for c in device_components]:
                self.logger.info(f"OS component '{component}' should not be present on the device")
                success = False

        if success:
            self.logger.info(f"PASS: OS '{os_version}' is an expected version")
        else:
            self.logger.info(f"FAIL: OS version '{os_version}' is not an expected version")

        return success

    def check_installed_images(self, expected_images):
        """
        Ensure device has correct images installed
        Parameters:
        expected_images: list of image ids for images that should be present
        """
        self.logger.info('Checking installed images...')

        success = True

        # Make Image IDs all lower case
        expected_images = [imageid.lower() for imageid in expected_images]

        # Make sure installed images matches expected images list exactly
        all_components = image.get_images()['components']
        filtered_components = [c for c in all_components if c['image_type'] == 10]
        device_components = list(
            map(lambda component: {"iid": component['images'][0]['uid'], "cid": component['uid'], "name": component['name']}, filtered_components))

        for img in device_components:
            component_id = img['cid'].lower()
            image_id = img['iid'].lower()
            if image_id in expected_images:
                expected_images.remove(image_id)
            else:
                self.logger.info(
                    f'Image ID: {image_id} (Component ID: {component_id}, {img["name"]}) not expected to be present on the device')
                success = False

        if len(expected_images) != 0:
            for img in expected_images:
                self.logger.info(f'Expected image ID {img} is not on the device')
            success = False

        if success:
            self.logger.info('PASS: Installed images matches expected images')
        else:
            self.logger.info('FAIL: Installed images do not match expected images')

        return success

    def perform_device_ready_checks(self, expected_mfg_state, expected_os_version,
                                    os_components_json_file, expected_additional_images):
        """
        Read configuration files and set up checker
        """
        device_ready = True

        # check manufacturing state
        device_manufacturing_state = self.get_manufacturing_state()
        if not self.check_manufacturing_state(expected_mfg_state, device_manufacturing_state):
            device_ready = False

        if device_manufacturing_state == "Blank":
            # Blank devices always have rftest capability enabled so this check always fails on blank devices
            self.logger.info('Checking capabilities...')
            self.logger.info(
                'SKIPPED: Capability check skipped for devices in Blank manufacturing state.')
        else:
            # check capabilities
            if not self.check_no_capabilities():
                device_ready = False

        if device_manufacturing_state == "DeviceComplete":
            # OS version check uses endpoint that is locked in DeviceComplete
            self.logger.info('Checking OS version...')
            self.logger.info(
                'SKIPPED: Device OS cannot be checked when device is in DeviceComplete state.')

            # Installed image check uses endpoint that is locked in DeviceComplete
            self.logger.info('Checking installed images...')
            self.logger.info(
                'SKIPPED: Installed images list cannot be checked when device is in DeviceComplete state.')

            # Wifi network check uses endpoint that is locked in DeviceComplete
            self.logger.info('Checking wifi networks...')
            self.logger.info(
                'SKIPPED: Wifi network list cannot be checked when device is in DeviceComplete state.')
        else:
            # check OS version
            if not self.check_os_version(expected_os_version, os_components_json_file):
                device_ready = False

            # check installed images
            if not self.check_installed_images(expected_additional_images):
                device_ready = False

            # check wifi
            if not self.check_no_wifi_networks():
                device_ready = False

        return device_ready


def create_formatted_logger(log_level):
    '''
    Helper function to create and return a formatted logger for the script
    '''

    class CustomLogFormatter(logging.Formatter):
        def __init__(self):
            super().__init__(datefmt='%Y-%m-%d %H:%M:%S')

        def format(self, record):
            if record.levelno == logging.INFO:
                self._style._fmt = "%(msg)s"
            else:
                self._style._fmt = "[%(asctime)s.%(msecs)03d][%(levelname)s] %(msg)s"

            return logging.Formatter.format(self, record)

    logger = logging.getLogger(__name__)
    logger.setLevel(log_level)
    console_handler = logging.StreamHandler()
    console_handler.setLevel(log_level)
    console_handler.setFormatter(CustomLogFormatter())
    logger.addHandler(console_handler)

    return logger

def get_argument_parser():
    '''
    Helper function to create and return an argument parser for the script
    '''
    parser = argparse.ArgumentParser(
        description='Check if device is configured correctly for shipping.')

    parser.add_argument('--os', nargs='*', default=[], type=str,
                        help='Space-separated list of OS versions that are allowed on the device.',
                        metavar="OSVERSION", dest="expected_os_version")
    parser.add_argument('--images', nargs='*', default=[], type=str,
                        help='Space-separated list of non-OS image IDs that should be present on the device.',
                        metavar="IMAGEID", dest="expected_additional_images")
    parser.add_argument('--os_components_json_file', '-j', default="mt3620an.json", type=str,
                        help='Path to the json file containing the lists of image and component IDs that define each OS version.',
                        metavar="PATH", dest="os_components_json_file")
    parser.add_argument('--expected_mfg_state', '-e', default="DeviceComplete",
                        choices=['Blank', 'Module1Complete', 'DeviceComplete'], type=str,
                        help='[Optional] Expected manufacturing state. Options are "Blank", '
                        '"Module1Complete", "DeviceComplete". Defaults to "DeviceComplete" if unspecified.',
                        dest="expected_mfg_state", metavar="STATE")
    parser.add_argument('--verbose', '-v', action='store_true', default=False,
                        help='Print verbose debugging information.')

    return parser


def validate_arguments(args, parser):
    '''
    Validate the script arguments
    '''
    if not os.path.exists(args.os_components_json_file):
        parser.error(f"Invalid value for os_components_json_file: '{args.os_components_json_file}' "
                     "does not exist or is not accessible.")


def confirm_device_connected(parser):
    device_list = devices.get_attached_devices()
    if device_list == 0:
        parser.error(f'No devices connected, device ready checks aborted')
        sys.exit(1)

def main():
    '''
    Main function of the script
    '''
    parser = get_argument_parser()
    args = parser.parse_args()
    validate_arguments(args, parser)

    if args.verbose:
        logger = create_formatted_logger(logging.DEBUG)
    else:
        logger = create_formatted_logger(logging.INFO)

    device_list = devices.get_attached_devices()
    if len(device_list) == 0:
        print('No devices connected, cannot continue with device ready checks.')
        sys.exit(1)
    try:
        checker = DeviceReadyChecker(logger)
        if checker.perform_device_ready_checks(
                expected_mfg_state=args.expected_mfg_state,
                expected_os_version=args.expected_os_version,
                os_components_json_file=args.os_components_json_file,
                expected_additional_images=args.expected_additional_images):
            logger.info('------------------')
            logger.info('PASS')
        else:
            logger.info('------------------')
            logger.info('FAIL: Device ready check not successful.')
            sys.exit(1)
    except Exception as e:
        logger.error(e)
        sys.exit(1)


if __name__ == '__main__':
    """
    Entry point for the script.
    """
    main()
