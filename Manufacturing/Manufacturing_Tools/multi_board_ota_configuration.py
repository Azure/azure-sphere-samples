#!/usr/bin/env python3

'''
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

multi_board_ota_configuration.py

This sample script demonstrates how to set up the cloud configuration for multiple Azure Sphere devices.
'''

import logging
import logging.config
import os
import sys
import argparse
from otaconfigurationdevice import OTAConfiguration


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
        description='Start claiming and configuring the product and device group for a list of devices')

    if sys.platform == 'win32':
        default_tools_path=r'C:\Program Files (x86)\Microsoft Azure Sphere SDK\Tools_v2\wbin\azsphere.cmd'
    if sys.platform == 'linux':
        default_tools_path=r'/opt/azurespheresdk/Tools_v2/azsphere'

    parser.add_argument('--azsphere_path', '-a', type=str,
                        default=default_tools_path,
                        help='Full path of the azsphere CLI tool.', metavar="PATH")
    parser.add_argument('--csv_file_path', '-c', default="OTAConfiguration.csv", type=str,
                        help='Path to the CSV file containing the lists of device IDs, product names and device group names.',
                        metavar="PATH")
    parser.add_argument('--azsphere_tenant', '-t', type=str,
                        help='The default tenant ID the devices will be claimed in.',
                        dest="azsphere_tenant", required=True, metavar="TENANT")
    parser.add_argument('--verbose', '-v', action='store_true', default=False,
                        help='Print verbose information.')
    parser.add_argument('--diag', '-d', action='store_true', default=False,
                        help='Print diagnostic information.')

    return parser


def validate_arguments(args, parser):
    '''
    Validate the script arguments
    '''
    if not os.path.exists(args.azsphere_path):
        parser.error(
            f"Invalid value for azsphere_path: '{args.azsphere_path}' does not exist or is not accessible.")
    if not os.path.exists(args.csv_file_path):
        parser.error(
            f"Invalid value for csv_file_path: '{args.csv_file_path}' does not exist or is not accessible.")


def main():
    '''
    Main function of the script
    '''
    parser = get_argument_parser()
    args = parser.parse_args()
    validate_arguments(args, parser)

    if args.diag:
        logger = create_formatted_logger(logging.DEBUG)
        verbose = True
    elif args.verbose:
        logger = create_formatted_logger(logging.INFO)
        verbose = True
    else:
        logger = create_formatted_logger(logging.INFO)
        verbose = False

    try:
        ota_cfg = OTAConfiguration(azsphere_path=args.azsphere_path,
                                   logger=logger, verbose=verbose)
        ota_cfg.set_default_tenant(args.azsphere_tenant)
        ota_cfg.read_devices_information(args.csv_file_path)
        ota_cfg.trigger_devices_claiming_process()

        logger.debug("\nFinished claiming and setting product name and device group\n")
    except Exception as e:
        logger.error(e)
        sys.exit(1)


if __name__ == '__main__':
    """
    Entry point for the script.
    """
    main()
