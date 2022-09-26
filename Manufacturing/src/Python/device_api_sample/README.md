# Sample for Microsoft Azure Sphere Device REST APIs for Python

This sample shows how to use Microsoft Azure Sphere Device REST APIs for Python. The sample gets and lists all the attached Azure Sphere devices.

The sample uses the following Microsoft Azure Sphere Device REST APIs.

| REST API | Purpose |
|---------|---------|
| [`get_attached_devices`](../../README.md#get-attached-devices) | Get the list of attached devices.  |
| [`get_device_security_state`](../../README.md#get-device-security-state) | Get the device security state. |
| [`set_active_device_ip_address`](../../README.md#set-active-device-ip-address) | Set the device IP address that the REST APIs should use. |

## Contents

| File/folder         | Description              |
|---------------------|--------------------------|
| display_ip_deviceid.py           | Python source code file. |

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits).

## Setup

1. Ensure that your Azure Sphere device(s) are connected to your computer.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.09 or above. At the command prompt, run `azsphere show-version` to check. If required, upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux).
1. Install the Microsoft Azure Sphere Device REST APIs for Python package from PyPI by running the command `pip install azuresphere_device_api`.

## Run the sample

To run this sample on Windows, run the command `python display_ip_deviceid.py`.

To run this sample on Linux, run the command `python3 display_ip_deviceid.py`.

### Observe the output

You will see output similar to below:
```
Azure Sphere Device API Sample.
Gets a list of attached devices, displays the IP address and Device ID
Device IpAddress: "192.68.35.2" , Device ID: "352fe1f59e40ef8a9266415e81af32b5b07d8f2bbd6b5650cef4a70b86c7fcbc70b129a41fbc6d02f8bb4aaabc52cd5740c85427d205e46a166b7e41135eb968"
```
