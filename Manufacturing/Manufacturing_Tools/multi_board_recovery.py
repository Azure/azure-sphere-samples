#!/usr/bin/env python3

'''
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

multi_board_recovery.py

This sample script demonstrates how to recover and sideload images onto multiple Azure Sphere
devices simultaneously.
'''

import subprocess
import argparse
import concurrent.futures
import os
import logging
import sys

from azuresphere import devices
from azuresphere.exceptions import AzureSphereDeviceApiException


class AzsphereCliHelper:
    """
    A command runner for azsphere.
    """

    def __init__(self, path):
        """
        path: the path to azsphere on the machine.
        """
        self.path = path

    def run_command(self, command=[]):
        """
        Run an azsphere command
        command: list of parameters to pass to azsphere

        Returns: the completed process object.
        """
        command_line = [self.path] + command

        try:
            return subprocess.run(
                command_line, stdout=subprocess.PIPE, universal_newlines=True, timeout=15*60)
        except subprocess.TimeoutExpired:
            return 'timeout'


class MultiBoardRecoveryAndSideloadHelper:
    def __init__(self, azsphere_cli_helper, logger):
        self.cli_helper = azsphere_cli_helper
        self.logger = logger

    def recover_device(self, device_location):
        """
        Recover a device.
        azsphere: an Azsphere executor.
        device_location: the location of the device.

        Returns: the completed process.
        """
        cmd = ['device', 'recover', '-d', device_location]
        return self.cli_helper.run_command(cmd)

    def get_devices(self):
        """
        Gets a list of the connected devices.

        Returns: a list of device locations.
        """
        values = devices.get_attached_devices()

        return values

    def sideload_imagepackage(self, azsphere_imagepackage_path, device_location):
        """
        Sideload an image package.

        azsphere: an Azsphere executor.
        azsphere_imagepackage_path: the path to the image package to sideload to the device.
        device_location: the location of the device.

        Returns: the completed process object.
        """
        cmd = ['device', 'sideload', 'deploy', '-p',
               azsphere_imagepackage_path, '-d', device_location]
        return self.cli_helper.run_command(cmd)

    def recover_device_and_sideload(self, azsphere_imagepackage_path, device_location):
        """
        Recover a device and then sideload and then optionally sideload an image.

        azsphere_imagepackage_path: the path to the image package to sideload to the device, or empty string.
        device_location: the location of the device.
        """
        recovery = self.recover_device(device_location)
        if recovery == 'timeout':
            self.logger.error(
                f'Recovery of device {device_location} failed with timeout.')
            return

        if recovery.returncode != 0:
            self.logger.error(
                f'Recovery of device {device_location} failed with error code {recovery.returncode}.')
            return

            # Do not print messages in cases of success, so that error messages are highly visible with many devices.
        if azsphere_imagepackage_path == '':
            return

        sideload = self.sideload_imagepackage(
            azsphere_imagepackage_path, device_location)
        if sideload == 'timeout':
            self.logger.error(
                f'Sideload to device {device_location} failed with timeout.')
            return

        if sideload.returncode != 0:
            self.logger.error(
                f'Sideload to device {device_location} failed with error code {sideload.returncode}.'
                ' Please ensure you are using a prod-signed image, rather than a development image.')
            return

    def test_connection_path(self, conn_path):
        print(conn_path)

    def schedule_parallel_recovery_and_sideload(self, azsphere_imagepackage_path):
        """
        Schedule parallel recovery-and-sideload for each device attached.

        azsphere: an Azsphere executor.
        azsphere_imagepackage_path: the path to the image package to sideload to the device, or empty string.
        logger: a logger
        """
        devices = self.get_devices()
        connection_paths = []
        for device in devices:
            connection_paths.append(device['DeviceConnectionPath'])

        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
            futureRecoveryMultiBoard = {executor.submit(
                self.recover_device_and_sideload, azsphere_imagepackage_path, device_connection_path): device_connection_path for device_connection_path in connection_paths}
            finished, pending = concurrent.futures.wait(
                futureRecoveryMultiBoard, None, concurrent.futures.ALL_COMPLETED)
            for task in finished:
                task.result()


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
        description='Perform multi-board recovery and sideload.')

    parser.add_argument('--azsphere_path', '-a', type=str,
                        default=r'C:\Program Files (x86)\Microsoft Azure Sphere SDK\Tools_v2\wbin\azsphere.cmd',
                        help='Full path of the azsphere CLI tool.', metavar="PATH")
    parser.add_argument('--imgpath', '-i', default='',
                        help='Full path of the image to sideload.')
    parser.add_argument('--verbose', '-v', action='store_true', default=False,
                        help='Print verbose debugging information.')

    return parser


def validate_arguments(args, parser):
    '''
    Validate the script arguments
    '''
    if not os.path.exists(args.azsphere_path):
        parser.error(
            f"Invalid value for azsphere_path: '{args.azsphere_path}' does not exist or is not accessible.")
    if args.imgpath != '' and not os.path.exists(args.imgpath):
        parser.error(
            f"Invalid value for imgpath: '{args.imgpath}' does not exist or is not accessible.")


def detect_os():
    if not sys.platform == "win32":
        print("Multi-board recovery is only supported on Windows")
        sys.exit(1)


def main():
    '''
    Main function of the script
    '''
    detect_os()
    parser = get_argument_parser()
    args = parser.parse_args()
    validate_arguments(args, parser)

    if args.verbose:
        logger = create_formatted_logger(logging.DEBUG)
    else:
        logger = create_formatted_logger(logging.INFO)

    try:
        cli_helper = AzsphereCliHelper(args.azsphere_path)
        recovery_and_sideload_helper = MultiBoardRecoveryAndSideloadHelper(
            cli_helper, logger)
        recovery_and_sideload_helper.schedule_parallel_recovery_and_sideload(
            args.imgpath)
    except Exception as e:
        logger.error(e)
        sys.exit(1)


if __name__ == '__main__':
    """
    Entry point for the script.
    """
    main()
