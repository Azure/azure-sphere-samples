# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import subprocess
import logging
import re
import logging.config
import csv
import random
import concurrent.futures
from azspherecli import AzsphereCliHelper
import json

class OTAConfigurationException(Exception):
    """
    Exception raised for errors when configuring OTA Devices.
    """

    def __init__(self, logger, message):
        logger.error(message)


class OTADevice:
    """
    Class used to maintain the OTA properties (product name, device group, tenant id) for a device
    """

    def __init__(self, tenant_id, device_id, product_name, device_group_name):
        self.tenant_id = tenant_id
        self.device_id = device_id
        self.prd_name = product_name
        self.dg_name = device_group_name

    def __str__(self):
        return f"\nTenantId:{self.tenant_id}\nDeviceId:{self.device_id}\nPrdName:{self.prd_name}\nDgName:{self.dg_name}\n"

    def __hash__(self):
        return int(self.device_id, 16)

    def __cmp__(self, other):
        if(self.device_id < other.device_id):
            return -1
        if(self.device_id == other.device_id):
            return 0
        if(self.device_id > other.device_id):
            return 1

    def __eq__(self, rhs):
        return self.device_id == rhs.device_id


class OTAConfiguration(AzsphereCliHelper):
    """
    Class used to claim the devices into the provided tenant and to set the product and device group for each device
    """

    def __init__(self, azsphere_path, logger, verbose=False):
        super().__init__(azsphere_path=azsphere_path, logger=logger, verbose=verbose, show_output=True)
        self.ota_device_set = set()
        self.default_tenant_id = 'Unknown'

    def _helper_check_return_value_or_raise_OTAConfigurationException(self, return_string, exceptionString):
        """
        Helper function to check the return value and raise an OTAConfiguration exception
        """
        if(return_string == 'Unknown' or return_string == 'UnknownMatch'):
            raise OTAConfigurationException(self.logger, exceptionString)
        else:
            self.logger.info("Done")

    def set_default_tenant(self, tenant_id):
        self.logger.info("Setting tenant...")
        self.default_tenant_id = self.show_default_tenant()

        if self.default_tenant_id != tenant_id:
            return_string = self.select_tenant(tenant_id)
        else:
            return_string = "SUCCESS"

        self._helper_check_return_value_or_raise_OTAConfigurationException(
            return_string, f"Couldn't select {tenant_id} as default tenant")

    def read_devices_information(self, path_to_device_information_csv):
        """
        Parses the CSV file and extracts the information about devices to be claimed
        """
        self.logger.info(
            f"Parsing OTA configuration CSV file '{path_to_device_information_csv}'...")
        expected_number_devices = 0
        with open(path_to_device_information_csv) as csv_file:
            read_csv = csv.DictReader(csv_file)
            for row in read_csv:
                expected_number_devices += 1
                self.ota_device_set.add(OTADevice(self.default_tenant_id,
                                                  row['DeviceId'], row['ProductName'], row['DeviceGroupName']))
            if(len(self.ota_device_set) != expected_number_devices):
                raise OTAConfigurationException(
                    self.logger,
                    f"Expected {expected_number_devices} unique device(s), instead read {len(self.ota_device_set)} unique device(s)")
        self.logger.info(f"Retrieved {len(self.ota_device_set)} unique devices to be claimed")

    def trigger_devices_claiming_process(self):
        """
        Creates a pool of threads which are going to claim the devices, assign them a product and a device group
        """
        if len(self.ota_device_set) < 1:
            raise OTAConfigurationException(
                self.logger,
                "The read_devices_information function should be called before triggering claiming\n")

        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
            future_to_ota_configuration = {executor.submit(
                self.configure_ota, ota_device): ota_device for ota_device in self.ota_device_set}
            finished, pending = concurrent.futures.wait(
                future_to_ota_configuration, None, concurrent.futures.FIRST_EXCEPTION)
            if len(pending) > 0:
                raise OTAConfigurationException(self.logger, "\nSome devices weren't claimed.\n")

    def configure_ota(self, ota_device):
        """
        Configures ota for one device: claims the device, assigns it a product and a device group
        """
        self.logger.info(f"Configuring OTA for device '{ota_device.device_id}'...")

        # claim device
        return_string = self.claim_device_into_default_tenant(ota_device)
        self._helper_check_return_value_or_raise_OTAConfigurationException(
            return_string, f"Couldn't claim {ota_device.device_id} into tenant {ota_device.tenant_id}.")
        self.logger.info(f"Claimed '{ota_device.device_id}'")

        products = self._helper_get_products_list()
        product_id=None
        for product in products:
            if product['name'].lower() == ota_device.prd_name.lower():
                product_id = product['id']

        device_group_info_json = self._helper_get_device_group_details(ota_device)
        # set product name and device group
        result = self.configure_device_prd_and_group(ota_device)
        result_json =  json.loads(result)
        if result_json['productId'] != product_id or result_json['deviceGroupId'] != device_group_info_json['id']:
            raise OTAConfigurationException(self.logger, f"Couldn't configure product {ota_device.prd_name} and device group {ota_device.dg_name} for device ID {ota_device.device_id}.")
        self.logger.info(f"Configured '{ota_device.device_id}'")


    # run command with json output (no need to parse the output)
    def _helper_run_azsphere_command_json(self, cmd):
        return super()._run_azsphere_command(cmd, ignore_error=False, json_output=True)

    def _helper_get_products_list(self):
        """
        Helper function to get all products in the tenant
        """
        cmd = ['product', 'list', '-o', 'json']
        d =  json.loads(self._helper_run_azsphere_command_json(cmd))               
        return d

    def _helper_get_device_group_details(self, ota_device):
        """
        Helper function to get device group information
        """
        cmd = ['device-group', 'show', 
               '--device-group', f'{ota_device.prd_name}/{ota_device.dg_name}', '-o', 'json']
        d =  json.loads(self._helper_run_azsphere_command_json(cmd))               
        return d

    def _helper_parse_azsphere_claim_output(self, cmd, string_to_search):
        """
        Helper function to parse the output
        """
        data = super()._run_azsphere_command(cmd, ignore_error=False)
        for line in data.split('\n'):
            line = line.strip()
            if string_to_search == line:
                return 'Done'
        return "Unknown"


    def show_default_tenant(self):
        """
        Get the default selected tenant for the existing environment
        """
        cmd = ['tenant', 'show-selected', '-o', 'json']
        d =  json.loads(self._helper_run_azsphere_command_json(cmd))
        return d['id']


    def select_tenant(self, tenant_id):
        """
        Select AzureSphere default tenant
        """
        self.default_tenant_id = tenant_id
        cmd = ['tenant', 'select', '-t', tenant_id, '-o', 'json']
        string_to_search = f"Selected Azure Sphere tenant is \'(.*)\' \({tenant_id}\)."
        d = json.loads(self._helper_run_azsphere_command_json(cmd))
        return d['id']


    def claim_device_into_default_tenant(self, ota_device):
        """
        Claim a device into the default tenant
        """
        cmd = ['device', 'claim', '-d', ota_device.device_id, '--force']
        string_to_search = f"Successfully claimed device ID \'{ota_device.device_id}\' into tenant with ID \'{ota_device.tenant_id}\'."
        return self._helper_parse_azsphere_claim_output(cmd, string_to_search)

    def configure_device_prd_and_group(self, ota_device):
        """
        Configure device product and group
        """
        cmd = ['device', 'update', '--device', ota_device.device_id,
               '--device-group', f'{ota_device.prd_name}/{ota_device.dg_name}', '-o', 'json']
        string_to_search = "Successfully moved device (.+)"
        return self._helper_run_azsphere_command_json(cmd)
