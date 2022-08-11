# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

from azuresphere_device_api import device, devices
from azuresphere_device_api.exceptions import AzureSphereDeviceApiException
from requests.exceptions import ConnectionError, RequestException, Timeout


def main():
    print("Azure Sphere Device API Sample.")
    print("Gets a list of attached devices, displays the IP address and Device ID")
    try:
        device_list = devices.get_attached_devices()
        if len(device_list) == 0:
            print("No device found")
            return

        for device_info in device_list:
            devices.set_active_device_ip_address(device_info['IpAddress'])
            security_state = device.get_device_security_state()
            print(
                f"Device IpAddress: {device_info['IpAddress']} , Device ID: {security_state['deviceIdentifier']}")
    except (AzureSphereDeviceApiException) as ex:
        print(ex)
    except ConnectionError as err:
        print("Error Connecting To Device:", err)
    except Timeout as err:
        print("Timeout Error To Device:", err)
    except RequestException as err:
        print("Requests Exception", err)


main()
