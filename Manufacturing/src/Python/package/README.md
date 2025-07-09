# Microsoft Azure Sphere Device REST APIs for Python

Microsoft Azure Sphere Device REST APIs for Python enables users to interact with an Azure Sphere device using REST APIs.

## Installation

You can find Microsoft Azure Sphere Device REST APIs for Python on [PyPi](https://pypi.org/project/azuresphere_device_api/).
1. If you haven't already, [install and/or upgrade the pip](https://pip.pypa.io/en/stable/installing/)
   of your Python environment to a recent version.
2. Run `pip install azuresphere_device_api`.

## Versions

This library follows [Semantic Versioning](http://semver.org/).

## Usage

Before using Microsoft Azure Sphere Device REST APIs for Python, you must install the Azure Sphere SDK.
- To install the Azure Sphere SDK on Windows, follow the [Windows Quickstart](https://learn.microsoft.com/azure-sphere/install/install-sdk?pivots=cli).
- To install the Azure Sphere SDK on Linux, follow the [Linux Quickstart](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux?pivots=cli-linux).

### Sample
The [display_ip_deviceid sample](https://github.com/Azure/azure-sphere-samples/blob/main/Manufacturing/src/Python/device_api_sample) gets the list of attached devices, displays the device IP address, and device ID.

### Active device IP address

By default, this package will target Azure Sphere devices with IP address `192.168.35.2`. To change the active Azure Sphere device, call `set_active_device_ip_address` API as below:
```
from azuresphere_device_api import devices
devices.set_active_device_ip_address("<Device_Ip_Address")
```

## Documentation

Microsoft Azure Sphere Device REST APIs for Python documentation is available at [Azure Sphere Device REST APIs Docs](https://github.com/Azure/azure-sphere-samples/blob/main/Manufacturing/src).

## Supported host operating systems

* Windows 10 and 11
* Ubuntu 18.04 and 20.04