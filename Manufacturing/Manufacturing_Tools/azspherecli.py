# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import subprocess
import logging
import re
import json


class AzsphereCliHelperException(Exception):
    """
    Exception thrown on errors being returned from calls to azsphere
    """
    pass


class AzsphereCliHelper:
    '''
    Helper class which wraps calls to the Azure Sphere command line interface - azsphere
    '''

    def __init__(self, azsphere_path, logger=logging.getLogger(__name__), verbose=False, show_output=True):
        """
        Create AzsphereCliHelper object
        azsphere_path: path to the azsphere executable
        Parameters:
        verbose: run azsphere in verbose mode
        show_output: print azsphere output to the terminal
        """
        self.azsphere_path = azsphere_path
        self.verbose = verbose
        self.show_output = show_output
        self.logger = logger

    def _run_azsphere_command(self, command=[], ignore_error=False, json_output=False):
        """
        Run an azsphere command
        command: list of parameters to pass to azsphere

        Returns: CompletedProcess object
        """
        command_line = [self.azsphere_path] + command

        if self.verbose:
            command_line += ['--verbose']

        self.logger.debug(f"Command to run: {' '.join(command_line)}")
        process = subprocess.run(command_line, stdout=subprocess.PIPE, universal_newlines=True)

        if self.show_output:
            for line in process.stdout.split('\n'):
                self.logger.debug(line)

        if json_output:
            data = process.stdout
        else:
            data = self._extract_data(process.stdout)

        if process.returncode != 0 and not ignore_error:
            raise AzsphereCliHelperException(f"Call to azsphere failed with messages: \n{data}")

        return data


    def _extract_data(self, output):
        """
        Helper function that filters warnings, verbose and diagnostic information lines from the
        output of the azsphere command.
        """
        lines = output.split('\n')
        data_lines = []
        for line in lines:
            match = re.match(r'Command completed.*in [0-9:.]+\.', line)

            if match:
                continue

            match = re.match('verbose:|warn:|trace:', line)

            if match:
                continue

            data_lines.append(line)
        return '\n'.join(data_lines)
