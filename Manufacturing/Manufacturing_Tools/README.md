# Azure Sphere Manufacturing Tools

These tools can assist with the manufacturing process by automating device readiness checks, recovering devices to a specific operating system version, deploying applications, and provisioning cloud configuration.
The [manufacturing documentation](https://learn.microsoft.com/azure-sphere/hardware/manufacturing-guide) describes how to use these tools as part of a manufacturing process.

The tools are written in Python, and require the [Azure Sphere SDK for Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Azure Sphere SDK for Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux).
These tools rely on the Python package for the Azure Sphere Device REST API client library. The source code for this library can be found at [../src](../src).  To use the scripts below, make sure you have used `pip` to install the client library.

There are four tools:

| Tool | Windows | Linux | Description |
|------|---------|-------|-------------|
| device_ready.py | Y | Y | Device readiness checks (os, applications, wifi, capabilities, manufacturing state) |
| download_os_list.py | Y | Y | Helper tool to download the current list of Azure Sphere OS versions |
| multi_board_ota_configuration.py | Y | Y | Tool to provision the cloud configuration for one or more devices |
| multi_board_recovery.py | Y | N | Recover multiple devices to a specific OS version, and optionally deploy applications |

Note that `azspherecli.py` and `otaconfigurationdevice.py` are helper libraries used by the tools listed above.

## Device readiness checks (`device_ready.py`)

The *device_ready.py* tool uses the Microsoft Azure Sphere Device REST APIs for Python package for communication to Azure Sphere devices.

The tool supports the following command line options:

| Arguments | Description |
|----------|---------|
| `--os` | List of expected operating systems for the device |
| `--images` | List of IDs (GUIDs) of the application images that are expected to be installed on the device. The list consists of the image GUIDs separated by spaces. |
| `--os_components_json_file` | The path for an Azure Sphere OS versions file (use the *download_os_list.py* tool to download the latest version)
| `--expected_mfg_state` | The expected manufacturing state (default `DeviceComplete`) |
| `--verbose` | Display more information when the tool is running |

Below is a sample run of the *device_ready.py* tool with the following arguments:
* `--os 22.11`
* `--os_components_json_file mt3620an.json`
* `--expected_mfg_state Module1Complete`

```cmd
device_ready.py --os 22.11 --os_components_json_file mt3620an.json --expected_mfg_state Module1Complete
Checking device is in manufacturing state Module1Complete...
PASS: Device manufacturing state is Module1Complete
Checking capabilities...
PASS: No capabilities on device
Checking OS version...
PASS: OS '22.11' is an expected version
Checking installed images...
PASS: Installed images matches expected images
Checking wifi networks...
PASS: Device has no wifi networks configured
------------------
PASS
```

Running the *device_ready.py* tool on a device that doesn't meet the input requirements would result in output similar to this:

```cmd
device_ready.py --os 22.11 --os_components_json_file mt3620an.json --expected_mfg_state
Checking device is in manufacturing state Module1Complete...
PASS: Device manufacturing state is Module1Complete
Checking capabilities...
PASS: No capabilities on device
Checking OS version...
PASS: OS '22.11' is an expected version
Checking installed images...
Image ID: 6415a939-c098-48cf-8e5c-02b52a9f0ffb (Component ID: 2f074b0f-d99d-4692-82d9-ef93a7d6463c, AzureSphereBlinkTest) not expected to be present on the device
FAIL: Installed images do not match expected images
Checking wifi networks...
PASS: Device has no wifi networks configured
------------------
FAIL: Device ready check not successful.
```

If the application deployed to the device is expected then you can run the tool with the following command line:

`device_ready.py --os 22.11 --os_components_json_file mt3620an.json --expected_mfg_state --images <GUIDs for expected images>`

## Download Azure Sphere OS Versions (`download_os_list.py`)

This tool downloads the latest Azure Sphere MT3620 operating systems list in JSON format. The JSON file contains a list of operating system versions and the list of component IDs and image IDs associated with a specific operating system release. The list of component and image IDs on a device (`azsphere device image list-installed --full`) can be compared against the operating systems JSON file to determine the OS version that's running on a device.

Note that the *device_ready.py* tool uses this model to determine a device operating system version.

The tool takes an optional path to save the downloaded mt3620an.json file, if the path is not provided the script will download the mt3620an.json file to the script folder.

## Multi-Board OTA configuration (`multi_board_ota_configuration.py`)

This tool enables bulk cloud configuration of devices, specifically claiming devices into a tenant, and associating devices with a given product and device group.

Note that this tool runs on Windows and Linux, requires the Azure Sphere SDK is installed, and should be run by a user with appropriate role that has logged into the Azure Sphere CLI (`azsphere login`).

The tool takes the following command line parameters:

| Arguments | Description |
|----------|---------|
| `--azsphere_path` | The default path for Windows and Linux will be used if not supplied |
| `--csv_file_path` | The CSV file contains a list of device IDs, product name, and device group name that devices will be associated with |
| `--azsphere_tenant` | The tenant ID (GUID) used for all device operations |
| `--verbose` | Display more information when the tool is running |
| `--diag` | Display tool debug information |

The CSV file doesn't require a specific name, and takes the following form:

Note that the first line `DeviceId,ProductName,DeviceGroupName` is the header row for the CSV file, the following rows contain the device ID, product name, and device group name.

```cmd
DeviceId,ProductName,DeviceGroupName
12345....54321,ContosoProduct,Development
54321....12345,ContosoProduct2,Production
```
Note that claiming a device into a tenant that it has already been claimed into will succeed.

Below is a sample run of the *multi_board_ota_configuration.py* tool with the following arguments:
* `--csv_file_path ota_test.csv`
* `--azsphere_tenant 44bcefec-8df5-4e3b-9f86-90e7af77c3b8`

Note that the ota_test.csv file used as an example only contained one row, multiple devices can be claimed/configured from one csv file.

```cmd
multi_board_ota_configuration.py --csv_file_path ota_test.csv --azsphere_tenant 44bcefec-8df5-4e3b-9f86-90e7af77c3b8
Setting tenant...
Done
Parsing OTA configuration CSV file 'ota_test.csv'...
Retrieved 1 unique devices to be claimed
Configuring OTA for device '574973887b27c57eb90a1c8e3640cc0c79fcb48b9160970bd3383767c98ca9af411e75807e41c9e878484eb856781369ecccd5b5a235f21a3294e23a733fd4ab'...DoneClaimed '574973887b27c57eb90a1c8e3640cc0c79fcb48b9160970bd3383767c98ca9af411e75807e41c9e878484eb856781369ecccd5b5a235f21a3294e23a733fd4ab'
Configured '574973887b27c57eb90a1c8e3640cc0c79fcb48b9160970bd3383767c98ca9af411e75807e41c9e878484eb856781369ecccd5b5a235f21a3294e23a733fd4ab'```
```

## Multi-Board recovery (`multi_board_recovery.py`)

The *multi_board_recovery.py* tool only runs on Windows, multiple connected Azure Sphere devices are supported - The Linux Azure Sphere SDK only supports one connected device. To recover a single device on Linux (or Windows), use the following Azure Sphere command to recover a device (`azsphere device recover`).

The tool takes the following command line parameters:

| Arguments | Description |
|----------|---------|
| `--azsphere_path` | The default path for Windows will be used if not supplied |
| `--imgpath` | Full path of the .imagepackage to sideload after recovery (optional) |

The *multi_board_recovery.py* tool runs the following Azure Sphere CLI commands for each connected device in parallel:
```cmd
azsphere device recover -d <connection path>
azsphere device sideload deploy -p <image_package_path/file-name> -d <connection path>
```

Running the tool with no command line parameters on Windows, with one connected device would display output similar to the following:

```cmd
WARNING: The device with IP '192.168.35.2' is selected from parameter '12113'
```
If errors occur during multi-board recover, or deployment of a .imagepackage file these will be displayed in the tool output.
