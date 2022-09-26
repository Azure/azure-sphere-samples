# Microsoft Azure Sphere Device REST APIs

 Microsoft Azure Sphere Device REST APIs enables users to interact with an Azure Sphere device using REST APIs.
 This is useful during [manufacturing operations](https://learn.microsoft.com/azure-sphere/hardware/manufacturing-guide).
 This folder contains two different implementations one in C# and the other in Python.
 These packages are published by Microsoft into PyPI and NuGet so they can be easily incorporated into Python or C# scripts.
 They are also used by the [manufacturing tools](../Manufacturing_Tools) Python scripts.

| Folder         | Description              |
|---------------------|--------------------------|
| CSharp           | Contains NuGet package source code and samples for Microsoft Azure Sphere Device REST APIs for C#. |
| Python           | Contains Python package source code and samples for Microsoft Azure Sphere Device REST APIs for Python. |

## REST API categories

| Category                                 | Description                                                 | C# Source File | Python Source File |
| ---------------------------------------- | ----------------------------------------------------------- | -------------- | ------------------ |
| [App](#app-apis)                         | Manage applications on the attached device.                 | [App.cs](./CSharp/Nuget/Package/App.cs)| [app.py](./Python/package/azuresphere_device_api/app.py) |
| [Capability](#capability-apis)           | View device capability configurations.                      | [Capabilities.cs](./CSharp/Nuget/Package/Capabilities.cs) | [capabilities.py](./Python/package/azuresphere_device_api/capabilities.py) |
| [Certificate](#certificate-apis)         | Manage certificates on the attached device.                 | [Certificate.cs](./CSharp/Nuget/Package/Certificate.cs) | [certificate.py](./Python/package/azuresphere_device_api/certificate.py) |
| [Device](#device-apis)                   | Manage Azure Sphere devices.                                | [Device.cs](./CSharp/Nuget/Package/Device.cs) | [device.py](./Python/package/azuresphere_device_api/device.py) |
| [Devices](#devices-apis)                 | List Azure Sphere devices connected to the local PC.   | [Devices.cs](./CSharp/Nuget/Package/Devices.cs) | [devices.py](./Python/package/azuresphere_device_api/devices.py) |
| [Image](#image-apis)                     | Manage device images.                                       | [Image.cs](./CSharp/Nuget/Package/Image.cs) | [image.py](./Python/package/azuresphere_device_api/image.py) |
| [Manufacturing](#manufacturing-apis)     | Manage the manufacturing state of attached devices.         | [Manufacturing.cs](./CSharp/Nuget/Package/Manufacturing.cs) | [manufacturing.py](./Python/package/azuresphere_device_api/manufacturing.py) |
| [Network](#network-apis)                 | Manage network interfaces on the attached device.           | [Network.cs](./CSharp/Nuget/Package/Network.cs) | [network.py](./Python/package/azuresphere_device_api/network.py) |
| [Sideload](#sideload-apis)               | Deploy and manage applications on the attached device.      | [Sideload.cs](./CSharp/Nuget/Package/Sideload.cs) | [sideload.py](./Python/package/azuresphere_device_api/sideload.py) |
| [Wi-Fi](#wi-fi-apis)                     | Manage Wi-Fi configurations for the attached device.        | [Wifi.cs](./CSharp/Nuget/Package/Wifi.cs) | [wifi.py](./Python/package/azuresphere_device_api/wifi.py) |



### App APIs

| App API                                           | Description                                                                | C# Function Name | Python Function Name |
| ------------------------------------------------- | -------------------------------------------------------------------------- |-------------- | ------------------ |
| [Get app quota](#get-app-quota)                   | Get the storage quota and usage for an application on the attached device. | [GetAppQuota](./CSharp/Nuget/Package/App.cs) | [get_app_quota](./Python/package/azuresphere_device_api/app.py) |
| [Get application status](#get-application-status) | Get the status of an application on the attached device.                   | [GetAppStatus](./CSharp/Nuget/Package/App.cs) | [get_app_status](./Python/package/azuresphere_device_api/app.py) |
| [Get memory statistics](#get-memory-statistics)   | Get the memory statistics for an application on the attached device.       | [GetMemoryStatistics](./CSharp/Nuget/Package/App.cs) | [get_memory_statistics](./Python/package/azuresphere_device_api/app.py) |
| [Set application status](#set-application-status) | Set the application status of an application on the attached device.       | [SetAppStatus](./CSharp/Nuget/Package/App.cs) | [set_app_status](./Python/package/azuresphere_device_api/app.py) |

### Get app quota

This function makes a `GET` request to `https://192.168.35.2/app/quota/{componentID}`, to show the storage quota and usage of the application on the attached device. `{componentID}` is the ID of the component to get the quota information for.

The Azure Sphere CLI command that calls this API is `azsphere device app show-quota`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.
```JSON
{"UsageKB":0,"LimitKB":0}
```

### Get application status

This function makes a REST `GET` call to `https://192.168.35.2/app/status/{componentID}`, to show the status of the application on the attached device. `{componentID}` is the ID of the component to show the status of.

The Azure Sphere CLI command that calls this API is `azsphere device app show-status`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{"state":"running"}
```

### Get memory statistics

This function makes a REST `GET` call to `https://192.168.35.2/stats/memory/groups/applications`, to show the memory statistics for applications on the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device app show-memory-stats`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "memoryStats": {
    "currentMemoryUsageInBytes": 147456,
    "peakUserModeMemoryUsageInBytes": 98304,
    "userModeMemoryUsageInBytes": 98304
  }
}
```

### Set application status

This function makes a REST `PATCH` call to `https://192.168.35.2/app/status/{componentID}`, to set the status of the application on the attached device. `{componentID}` is the ID of the component to set the status of.
The three states for the `trigger` parameter are `start`, `stop`, and `startDebug`.

The Azure Sphere CLI commands that call this API are `azsphere device app start` and `azsphere device app stop`.

#### Example response

The following JSON is the example response for a successful HTTP/SET request.

```JSON
{"state":"stopped"}
```

### Certificate APIs

| Certificate API                                 | Description                                                              | C# Function Name | Python Function Name |
| ----------------------------------------------- | ------------------------------------------------------------------------ | -------------- | ------------------ |
| [Add certificate](#add-certificate)             | Add a certificate to the attached device's certificate store.            | [AddCertificate](./CSharp/Nuget/Package/Certificate.cs) | [add_certificate](./Python/package/azuresphere_device_api/certificate.py) |
| [Get all certificates](#get-all-certificates)   | Get all certificate IDs in the attached device's certificate store.      | [GetAllCertificates](./CSharp/Nuget/Package/Certificate.cs) | [get_all_certificates](./Python/package/azuresphere_device_api/certificate.py) |
| [Get certificate](#get-certificate)             | Get details of a certificate in the attached device's certificate store. | [GetCertificate](./CSharp/Nuget/Package/Certificate.cs) | [get_certificate](./Python/package/azuresphere_device_api/certificate.py) |
| [Get certificate space](#get-certificate-space) | Get the available free space in the attached device's certificate store. | [GetCertificateSpace](./CSharp/Nuget/Package/Certificate.cs) | [get_certificate_space](./Python/package/azuresphere_device_api/certificate.py) |
| [Remove certificate](#remove-certificate)       | Delete a certificate in the attached device's certificate store.         | [RemoveCertificate](./CSharp/Nuget/Package/Certificate.cs) | [remove_certificate](./Python/package/azuresphere_device_api/certificate.py) |

### Add certificate

This function makes a REST `POST` call to `https://192.168.35.2/certstore/certs/{identifier}`, to add a certificate to the attached device's certificate store. Here `{identifier}` is the ID of the certificate for which to add.

The Azure Sphere CLI command that calls this API is `azsphere device certificate add`.

#### Example response

The following JSON is the example response for a successful HTTP/POST request.

```JSON
{}
```

### Get all certificates

Thi function makes a REST `GET` call to `https://192.168.35.2/certstore/certs`, to list certificates in the attached device's certificate store.

The Azure Sphere CLI command that calls this API is `azsphere device certificate list`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{"identifiers":["TestCert1","TestCert2"]}
```

### Get certificate

This function makes a REST `GET` call to `https://192.168.35.2/certstore/certs/{identifier}`, to show details of a certificate in the attached device's certificate store. Here `{identifier}` is the ID of the certificate for which to show the details.

The Azure Sphere CLI command that calls this API is `azsphere device certificate show`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "id": "testCA",
  "notBefore": "2022-04-12T08:31:54",
  "notAfter": "2023-04-12T08:51:54",
  "subjectName": "/CN=<TempRootCertificate>",
  "issuerName": "/CN=<TempRootCertificate>"
}
```

### Get certificate space

This function makes a REST `GET` call to `https://192.168.35.2/certstore/space`, to show the available free space in the attached device's certificate store.

The Azure Sphere CLI command that calls this API is `azsphere device certificate show-quota`.

The following JSON is the example response for a successful HTTP/GET request, the result is in bytes.

```JSON
{"AvailableSpace":"24514"}
```

### Remove certificate

This function makes a REST `DELETE` call to `https://192.168.35.2/certstore/certs/{identifier}`, to delete a certificate in the attached device's certificate store. Here `{identifier}` is the ID of the certificate for which to delete.

The Azure Sphere CLI command that calls this API is `azsphere device certificate delete`.

#### Example response

The following JSON is the example response for the HTTP/DELETE request.
```JSON
{}
```

### Capability APIs

| Capabilities API                                    | Description                                                             | C# Function Name | Python Function Name |
| --------------------------------------------------- | ----------------------------------------------------------------------- | -------------- | ------------------ |
| [Get device capabilities](#get-device-capabilities) | Get the current device capability configuration of the attached device. | [GetDeviceCapabilities](./CSharp/Nuget/Package/Capabilities.cs) | [get_device_capabilities](./Python/package/azuresphere_device_api/capabilities.py) |

### Get device capabilities

This function makes a REST `GET` call to `https://192.168.35.2/device/capabilities`, to get the current device capability configuration of the attached device.

A device can have the following capabilities:
- [`appDevelopment`](https://learn.microsoft.com/azure-sphere/app-development/device-capabilities#the-appdevelopment-capability) with a value of `11`
- [`enableRFTestMode`](https://learn.microsoft.com/azure-sphere/app-development/device-capabilities#the-enablerftestmode-capability) with a value of `12`
- [`fieldServicing`](https://learn.microsoft.com/azure-sphere/app-development/device-capabilities#the-fieldservicing-capability) with a value of `13`

The Azure Sphere CLI command that calls this API is  `azsphere device capability show-attached`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request:

```JSON
{"device_capabilities":[11]}
```

### Device APIs

| Device API                                                            | Description                                           | C# Function Name | Python Function Name |
| ----------------------------------------------------------------------| ----------------------------------------------------- | -------------- | ------------------ |
| [Clear error report data](#clear-error-report-data)                   | Clear error report data.                              | [ClearErrorReportData](./CSharp/Nuget/Package/Device.cs) | [clear_error_report_data](./Python/package/azuresphere_device_api/device.py) |
| [Get device REST API version](#get-device-rest-api-version)           | Get the version of REST API.                          | [GetDeviceRestAPIVersion](./CSharp/Nuget/Package/Device.cs) | [get_device_rest_api_version](./Python/package/azuresphere_device_api/device.py) |
| [Get device OS version](#get-device-os-version)                       | Get the version of the OS.                            | [GetDeviceOSVersion](./CSharp/Nuget/Package/Device.cs) | [get_device_os_version](./Python/package/azuresphere_device_api/device.py) |
| [Get device security state](#get-device-security-state)               | Get the device security state.                         | [GetDeviceSecurityState](./CSharp/Nuget/Package/Device.cs) | [get_device_security_state](./Python/package/azuresphere_device_api/device.py) |
| [Get device status](#get-device-status)                               | Get the device status.                               | [GetDeviceStatus](./CSharp/Nuget/Package/Device.cs) | [get_device_status](./Python/package/azuresphere_device_api/device.py) |
| [Get diagnostic log](#get-diagnostic-log)                             | Get the diagnostics log.                              | [GetDiagnosticLog](./CSharp/Nuget/Package/Device.cs) | [get_diagnostic_log](./Python/package/azuresphere_device_api/device.py) |
| [Get error report data](#set-error-report-data)                       | Get the device error report.                          | [GetErrorReportData](./CSharp/Nuget/Package/Device.cs) | [get_error_report_data](./Python/package/azuresphere_device_api/device.py) |
| [Restart device](#restart-device)                                     | Restart the attached device.                          | [RestartDevice](./CSharp/Nuget/Package/Device.cs) | [restart_device](./Python/package/azuresphere_device_api/device.py) |

### Clear error report data

This function makes a `DELETE` request to `https://192.168.35.2/telemetry`, to clear error report data.

The Azure Sphere CLI command that calls this API is  `azsphere get-support-data`.

#### Example response
```
{}
```

### Get device REST API version

This function makes a `GET` request to `https://192.168.35.2/status`, to get REST API version on the attached device. This request is made purely to get the HTTP Header that contains the REST API version information.

There is no Azure Sphere CLI command corresponding to this API call.

#### Example response
```
{"REST-API-Version":"4.4.0"}
```

### Get device OS version

This function makes a `GET` request to `https://192.168.35.2/osversion`, to get the operating system (OS) version on the attached device.

The Azure Sphere CLI command that uses this API is `azsphere device show-os-version`.

#### Example response

```
{"osversion":"22.09"}
```

### Get device security state

This function makes a REST `GET` call to `https://192.168.35.2/device/security_state`, to get the security state on the attached device.

There is no Azure Sphere CLI command corresponding to this API call.

#### Example response

The following JSON is the example response for a successful HTTP/GET request for anonymous authentication type.

```JSON
{
  "securityState": "SECURE",
  "deviceIdentifier": "352fe1f59e40ef8a9266415e81af32b5b07d8f2bbd6b5650cef4a70b86c7fcbc70b129a41fbc6d02f8bb4aaabc52cd5740c85427d205e46a166b7e41135eb968",
  "deviceIdentityPublicKey": "352fe1f59e40ef8a9266415e81af32b5b07d8f2bbd6b5650cef4a70b86c7fcbc70b129a41fbc6d02f8bb4aaabc52cd5740c85427d205e46a166b7e41135eb968"
}
```

### Get device status

This function makes a `GET` request to `https://192.168.35.2/status`, to get the status of the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device restart`.

#### Example response
```
{ "uptime": 634 }
```

### Get diagnostic log

This function makes a REST `GET` call to `https://192.168.35.2/log`, to gather diagnostic data about your device configurations.
There are no public tools to decode this log, and you would need to send this log to Microsoft to decode the logs. Refer to [Azure Sphere Support](https://learn.microsoft.com/azure-sphere/resources/support) docs.

The Azure Sphere CLI command that calls this API is `azsphere get-support-data`.

The result of the HTTP/GET request is a byte array.

### Get error report data

This function makes a `GET` request to `https://192.168.35.2/telemetry`, to retrieve error report data for the attached device.

The results of the HTTP/GET request is a byte array.

The Azure Sphere CLI command that calls this API is  `azsphere get-support-data`.

### Restart device

This function makes a `POST` request with no body to `https://192.168.35.2/restart` to restart the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device restart`

#### Example response
```
{ "restartingSystem": true }
```

### Devices APIs

| Devices API                                                           | Description                                                     | C# Function Name | Python Function Name |
| ----------------------------------------------------------------------| --------------------------------------------------------------  | -------------- | ------------------ |
| [Get active device IP address](#get-active-device-ip-address)         | Get the active device IP address                       .        | [GetActiveDeviceIpAddress](./CSharp/Nuget/Package/Devices.cs) | [get_active_device_ip_address](./Python/package/azuresphere_device_api/devices.py) |
| [Get attached devices](#get-attached-devices)                         | Get the list of attached devices.                               | [GetAttachedDevices](./CSharp/Nuget/Package/Devices.cs) | [get_attached_devices](./Python/package/azuresphere_device_api/devices.py) |
| [Set active device IP address](#set-active-device-ip-address)         | Set the device IP address that the REST APIs should use.        | [SetActiveDeviceIpAddress](./CSharp/Nuget/Package/Devices.cs) | [set_active_device_ip_address](./Python/package/azuresphere_device_api/devices.py) |

### Get active device IP address

This function gets the IP address of the device that the REST APIs use when multiple devices are attached.
Note: Multiple devices can be connected on Windows only.

### Get attached devices

Windows: This function makes a REST `GET` call to `http://localhost:48938/api/service/devices`, to get the list the attached devices.
Linux: This function checks the network adapter status to determine whether there is an attached device.

The Azure Sphere CLI commands that calls this API are `azsphere device list-attached` and `azsphere device show-attached`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

Windows:
```JSON
[
  {
    "IpAddress": "192.168.35.2",
    "DeviceConnectionPath": "1243"
  }
]
```

Linux:
```JSON
[
  {
    "IpAddress": "192.168.35.2",
    "DeviceConnectionPath": ""
  }
]
```

### Set active device IP address

This function sets the IP address of the device to run the command on when multiple devices are attached. Multi-board is supported only on Windows.

### Image APIs

| Image API                         | Description                                       | C# Function Name | Python Function Name |
| --------------------------------- | ------------------------------------------------- | -------------- | ------------------ |
| [Get all images](#get-all-images) | List the images currently on the attached device. | [GetImages](./CSharp/Nuget/Package/Image.cs) | [get_images](./Python/package/azuresphere_device_api/image.py) |

### Get all images

This function makes a REST `GET` call to `https://192.168.35.2/images`, to list the images currently on the attached device.

The Azure Sphere CLI command that calls this API is  `azsphere device image list-installed --full`.

#### Example response

The following JSON is an example of a partial response for a successful HTTP/GET request:
```JSON
{
  "is_ota_update_in_progress": false,
  "has_staged_updates": false,
  "restart_required": false,
  "components": [
    {
      "uid": "ec966946b-893b-4ff2-9ef1-b41725a8c652",
      "image_type": 7,
      "is_update_staged": false,
      "does_image_type_require_restart": true,
      "images": [
        {
          "uid": "3ba893921-535b-1684-b0a8-05ca4a30934a",
          "length_in_bytes": 2544816,
          "uncompressed_length_in_bytes": 2544816,
          "replica_type": 0
        }
      ],
      "name": "NW Kernel"
    }
  ]
}
```

See the [Image - Get Metadata](https://learn.microsoft.com/rest/api/azure-sphere/image/get-metadata) for more information on the image type.

### Manufacturing APIs

| Manufacturing API                                   | Description                                         | C# Function Name | Python Function Name |
| --------------------------------------------------- | --------------------------------------------------- | -------------- | ------------------ |
| [Get manufacturing state](#get-manufacturing-state) | Get the manufacturing state of the attached device. | [GetManufacturingState](./CSharp/Nuget/Package/Manufacturing.cs) | [get_device_manufacturing_state](./Python/package/azuresphere_device_api/manufacturing.py) |
| [Set manufacturing state](#set-manufacturing-state) | Set the manufacturing state of the attached device. | [SetDeviceManufacturingState](./CSharp/Nuget/Package/Manufacturing.cs) | [set_device_manufacturing_state](./Python/package/azuresphere_device_api/manufacturing.py) |

### Get manufacturing state

This function makes a REST `GET` call to `https://192.168.35.2/device/manufacturing_state`, to show the manufacturing state of the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device manufacturing-state show`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{"manufacturingState":"Module1Complete"}
```

### Set manufacturing state

This function makes a REST `PUT` call to `https://192.168.35.2/device/manufacturing_state`, to update the manufacturing state of the attached device.
NOTE: Set the manufacturing state with caution. This action cannot be reversed.

There are four manufacturing states: `Blank`, `Module1Complete`, `DeviceComplete`, and `Unknown`. The default state that the device ships in is `Blank`. The two states that you can set with this command are `Module1Complete` and `DeviceComplete`.

The Azure Sphere CLI command that calls this API is `azsphere device manufacturing-state update`.

#### Example response

The following JSON is the example response for performing the HTTP/PUT request.

```JSON
{}
```

### Network APIs

| Network API                                                                 | Description                                             | C# Function Name | Python Function Name |
| --------------------------------------------------------------------------- | ------------------------------------------------------- | -------------- | ------------------ |
| [`Configure proxy`](#configure-proxy)                                       | Configure the network proxy on the attached device.     | [ConfigureProxy](./CSharp/Nuget/Package/Network.cs) | [configure_proxy](./Python/package/azuresphere_device_api/network.py) |
| [`Delete network proxy`](#delete-network-proxy)                             | Delete proxy connection on the attached device.         | [DeleteNetworkProxy](./CSharp/Nuget/Package/Network.cs) | [delete_network_proxy](./Python/package/azuresphere_device_api/network.py) |
| [`Get all network connection failure logs`](#get-all-network-interfaces)    | List the network interfaces for the attached device.    | [GetAllFailedNetworkConnections](./CSharp/Nuget/Package/Network.cs) | [get_all_failed_network_connections](./Python/package/azuresphere_device_api/network.py) |
| [`Get all network interfaces`](#get-all-network-interfaces)                 | List the network interfaces for the attached device.    | [GetAllNetworkInterfaces](./CSharp/Nuget/Package/Network.cs) | [get_all_network_interfaces](./Python/package/azuresphere_device_api/network.py) |
| [`Get network connection failure log`](#get-network-connection-failure-log) | Get a specific log of a failed network connection.      | [GetFailedNetworkConnection](./CSharp/Nuget/Package/Network.cs) | [get_failed_network_connection](./Python/package/azuresphere_device_api/network.py) |
| [`Get network firewall ruleset`](#get-network-firewall-ruleset)             | List firewall rules for the attached device.            | [GetNetworkFirewallRuleset](./CSharp/Nuget/Package/Network.cs) | [get_network_firewall_ruleset](./Python/package/azuresphere_device_api/network.py) |
| [`Get network interface`](#get-network-interface)                           | Show a network interface on the attached device.        | [GetNetworkInterface](./CSharp/Nuget/Package/Network.cs) | [get_network_interface](./Python/package/azuresphere_device_api/network.py) |
| [`Get network proxy`](#get-network-proxy)                                   | Show proxy connection on the attached device.           | [GetNetworkProxy](./CSharp/Nuget/Package/Network.cs) | [get_network_proxy](./Python/package/azuresphere_device_api/network.py) |
| [`Get network status`](#get-network-status)                                 | Show the network status for the attached device.        | [GetNetworkStatus](./CSharp/Nuget/Package/Network.cs) | [get_network_status](./Python/package/azuresphere_device_api/network.py) |
| [`Set network interfaces`](#set-network-interfaces)                         | Configure the network interface on the attached device. | [SetNetworkInterfaces](./CSharp/Nuget/Package/Network.cs) | [set_network_interfaces](./Python/package/azuresphere_device_api/network.py) |


### Configure proxy

This function makes a REST `POST` call to `https://192.168.35.2/net/proxy`, to configure the network proxy on the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device network proxy apply`.

#### Example response

The following JSON is the example response for a successful HTTP/POST request for anonymous authentication:

```JSON
{
  "address": "192.168.98.3",
  "enabled": true,
  "port": 123,
  "authenticationType": "anonymous",
  "noProxyAddresses": []
}
```

See the [proxy code snippets](https://learn.microsoft.com/azure-sphere/network/connect-through-a-proxy#samples), to learn more about interacting with a proxy on an Azure Sphere device.

### Delete network proxy

This function makes a REST `DELETE` call to `https://192.168.35.2/net/proxy`, to delete the proxy connection on the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device network proxy delete`.

#### Example response

An example for a successful HTTP/DELETE request is as below.
```JSON
{}
```

### Get all network connection failure logs

This function makes a REST `GET` call to `https://192.168.35.2/wifi/diagnostics/networks`, to get logs of all failed network connection attempts.

The Azure Sphere CLI command that calls this API is `azsphere get-support-data`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

````JSON
{
  "values": [
    {
      "timestamp": "2000-01-01t00:04:37+0000",
      "networkId": 0,
      "ssid": "ExampleNetworkName",
      "error": "NetworkNotFound",
      "configState": "disabled",
      "connectionState": "disconnected"
    }
  ]
}
````

### Get all network interfaces

This function makes a REST `GET` call to `https://192.168.35.2/net/interfaces`, to list the network interfaces for the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device network list-interfaces`.

#### Example response

An example for a successful HTTP/GET request is as below.
```JSON
{
  "interfaces": [
    {
      "interfaceName": "azspheresvc",
      "interfaceUp": true,
      "connectedToNetwork": false,
      "ipAcquired": false,
      "connectedToInternet": false,
      "ipAddresses": [
        "192.168.35.2"
      ]
    },
    {
      "interfaceName": "lo",
      "interfaceUp": true,
      "connectedToNetwork": false,
      "ipAcquired": false,
      "connectedToInternet": false,
      "ipAddresses": [
        "127.0.0.1"
      ]
    }
  ]
}
```

### Get network connection failure log

This function makes a REST `GET` call to `https://192.168.35.2/wifi/diagnostics/networks/{id}`, to get the failed network connection attempts of a named network. `{id}` is the ID of the network for which you want get the logs.

The Azure Sphere CLI command that calls this API is `azsphere get-support-data`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "timestamp": "2000-01-01t00:04:37+0000",
  "networkId": 0,
  "ssid": "ExampleNetworkName",
  "error": "NetworkNotFound",
  "configState": "disabled",
  "connectionState": "disconnected"
}
```

### Get network firewall ruleset

This function makes a REST `GET` call to `http://192.168.35.2/net/firewall/rulesets`, to list firewall rules for the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device network list-firewall-rules`.

#### Example response

An example for a successful HTTP/GET request is as below.

```JSON
{
  "rulesets": [
    {
      "hook": "PREROUTING",
      "isValid": false,
      "rules": []
    },
    {
      "hook": "INPUT",
      "isValid": true,
      "rules": [
        {
          "sourceIP": "0.0.0.0",
          "sourceMask": "0.0.0.0",
          "destinationIP": "127.0.0.1",
          "destinationMask": "255.255.255.255",
          "uid": 4292567295,
          "action": "accept",
          "interfaceInName": "",
          "interfaceOutName": "",
          "state": "none",
          "tcpMask": [],
          "tcpCmp": [],
          "tcpInv": false,
          "protocol": "any",
          "sourcePortRange": {
            "min": 0,
            "max": 65535
          },
          "destinationPortRange": {
            "min": 0,
            "max": 65535
          },
          "packets": 3,
          "bytes": 249
        }
      ]
    }
  ]
}
```

### Get network interface

This function makes a REST `GET` call to `https://192.168.35.2/net/interfaces/{networkInterfaceName}`, to get the status of network interfaces. `{networkInterfaceName}` is the name of the network interface for which to get the status.

There is no Azure Sphere CLI command associated with this API call.

#### Example response

The following JSON is the example response for a successful HTTP/GET request for an interface with name `wlan0`.

```JSON
{
  "interfaceName": "wlan0",
  "interfaceUp": true,
  "connectedToNetwork": true,
  "ipAcquired": true,
  "connectedToInternet": true,
  "hardwareAddress": "1d:d8:47:fe:f7:33",
  "ipAssignment": "dynamic",
  "ipAddresses": [
    "172.20.10.3"
  ]
}
```

### Get network proxy

This function makes a REST `GET` call to `https://192.168.35.2/net/proxy`, to get the proxy connection on the attached device.

The Azure Sphere CLI command that calls this API is `azsphere device network proxy show`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.
```JSON
{
  "address": "192.168.98.3",
  "enabled": true,
  "port": 123,
  "authenticationType": "anonymous",
  "noProxyAddresses": []
}
```

### Get network status

This function makes a REST `GET` call to `https://192.168.35.2/net/status`, to get the network status of the device.

The Azure Sphere CLI command that calls this API is `azsphere device network show-status`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "deviceAuthenticationIsReady": true,
  "networkTimeSync": "complete",
  "proxy": "disabled"
}
```

### Set network interfaces

This function makes a REST `PATCH` call to `https://192.168.35.2/net/interfaces/{networkInterfaceName}`, to update the network interface configuration for the attached device. `{networkInterfaceName}` is the name of the interface that is to be updated.
The list of current network interface names can be obtained by calling the [`Get all network interfaces` function](#get-all-network-interfaces).

The Azure Sphere CLI command that calls this API is `azsphere device network update-interface`.

NOTE: This command can only be executed on an ethernet adapter, not on Wi-Fi.

#### Example response

The following JSON is the example response for a successful HTTP/PATCH request.
```JSON
{}
```

### Sideload APIs

| Sideload API                      | Description                              | C# Function Name | Python Function Name |
| --------------------------------- | ---------------------------------------- | -------------- | ------------------ |
| [Delete image](#delete-image)     | Delete an image on the device.           | [DeleteImage](./CSharp/Nuget/Package/Sideload.cs) | [delete_image](./Python/package/azuresphere_device_api/sideload.py) |
| [Install images](#install-images) | Install all staged images on the device. | [InstallImages](./CSharp/Nuget/Package/Sideload.cs) | [install_images](./Python/package/azuresphere_device_api/sideload.py) |
| [Stage image](#stage-image)       | Stage an image on the device.            | [StageImage](./CSharp/Nuget/Package/Sideload.cs) | [stage_image](./Python/package/azuresphere_device_api/sideload.py) |

### Delete image

This function makes a REST `DELETE` call to `https://192.168.35.2/app/image/{componentID}`, to delete an image from a device. `{componentID}` is the component ID of the component (application) on your device.

The Azure Sphere CLI command that calls this API is `azsphere device app delete`.

#### Example response

The following JSON is the example response for a successful HTTP/DELETE request.

```JSON
{}
```

### Install images

This function makes a REST `POST` call to `https://192.168.35.2/update/install`, to install an image on the attached device.
An application can be installed with an `appControlMode` of either `Auto` or `Manual`. `Auto` starts the application automatically after installation while `Manual` requires a manual restart.
You need to first stage the image, before installing. See [Stage image](#stage-image)

The Azure Sphere CLI command this API calls is `azsphere device sideload deploy`.

#### Example response

The following JSON is the example response for a successful HTTP/POST request.

```JSON
{}
```


### Stage image

This function makes a REST `PUT` call to `https://192.168.35.2/update/stage`, to stage image on the attached devices. The content of the image package is sent as the body in the REST call.

The Azure Sphere CLI command that calls this API is `azsphere device sideload deploy`.

#### Example response

The following JSON is the example response for a successful HTTP/PUT request.

```JSON
{}
```

### Wi-Fi APIs

| Wi-Fi API                                                                 | Description                                                         | C# Function Name | Python Function Name |
| ------------------------------------------------------------------------- | ------------------------------------------------------------------- | -------------- | ------------------ |
| [`Add configured Wi-Fi network`](#add-configured-wi-fi-network)           | Add a Wi-Fi network on the attached device.                         | [AddWifiNetwork](./CSharp/Nuget/Package/Wifi.cs) | [add_wifi_network](./Python/package/azuresphere_device_api/wifi.py) |
| [`Change configured Wi-Fi network`](#change-configured-wi-fi-network)     | Change the configuration state of a network on the attached device. | [ChangeConfiguredWifiNetwork](./CSharp/Nuget/Package/Wifi.cs) | [change_wifi_network_config](./Python/package/azuresphere_device_api/wifi.py) |
| [`Change Wi-Fi interface state`](#change-wi-fi-interface-state)           | Change the status of the wireless interface on the attached device. | [ChangeConfiguredWifiNetwork](./CSharp/Nuget/Package/Wifi.cs) | [change_wifi_interface_state](./Python/package/azuresphere_device_api/wifi.py) |
| [`Get all configured Wi-FI networks`](#get-all-configured-wi-fi-networks) | List the current Wi-Fi configurations for the attached device.      | [GetAllConfiguredWifiNetworks](./CSharp/Nuget/Package/Wifi.cs) | [get_all_wifi_networks](./Python/package/azuresphere_device_api/wifi.py) |
| [`Get configured Wi-Fi network`](#get-configured-wi-fi-network)           | Get a Wi-Fi network on the attached device.                         | [GetConfiguredWifiNetwork](./CSharp/Nuget/Package/Wifi.cs) | [get_configured_wifi_network](./Python/package/azuresphere_device_api/wifi.py) |
| [`Get Wi-Fi interface state`](#get-wi-fi-interface-state)                 | Get the state of the wireless interface on the attached device.     | [GetWiFiInterfaceState](./CSharp/Nuget/Package/Wifi.cs) | [get_wifi_interface_state](./Python/package/azuresphere_device_api/wifi.py) |
| [`Get Wi-Fi scan results`](#get-wi-fi-scan-results)                       | Scan for available Wi-Fi networks visible to the attached device.   | [GetWiFiScan](./CSharp/Nuget/Package/Wifi.cs) | [get_wifi_scan](./Python/package/azuresphere_device_api/wifi.py) |
| [`Remove configured Wi-Fi network`](#remove-configured-wi-fi-network)     | Forget a Wi-Fi network on the attached device.                      | [DeleteWiFiNetConfig](./CSharp/Nuget/Package/Wifi.cs) | [remove_configured_wifi_network](./Python/package/azuresphere_device_api/wifi.py) |

### Add configured Wi-Fi network

This functionmakes a REST `GET` call to `https://192.168.35.2/wifi/config/networks`, to add a Wi-Fi network on the attached device.

The CLI command this API calls is `azsphere device wifi add`.

#### Example response

```JSON
{
  "ssid": "ExampleNetworkName",
  "configState": "enabled",
  "connectionState": "unknown",
  "id": 0,
  "securityState": "psk",
  "targetedScan": false
}
```

### Change configured Wi-Fi network

This function makes a REST `PATCH` call to `https://192.168.35.2/wifi/config/networks/{id}`, to modify a Wi-Fi network configuration. `{id}` is the ID of the Wi-Fi network configuration you would like to modify.
`configState` can take one of four values: `unknown`, `enabled`, `disabled`, `temp-disabled`.

There is no Azure Sphere CLI command associated with this API call.

#### Example response

The following JSON is the example response for a successful HTTP/PATCH request.

```JSON
{
  "ssid": "ExampleNetworkName",
  "configState": "disabled",
  "connectionState": "disconnected",
  "id": 0,
  "securityState": "psk",
  "targetedScan": false
}
```

### Change Wi-Fi interface state

This function makes a REST `PATCH` request to `https://192.168.35.2/wifi/interface`, to change the Wi-Fi interface state on the attached device.
Note: The state might take a while to change, and hence you might need to poll to see the state change.

The Azure Sphere CLI commands this API calls are `azsphere device wifi enable` and `azsphere device wifi disable`.

#### Example response

The following JSON is the example response for a successful HTTP/PATCH request.

```JSON
{}
```

### Get all configured Wi-Fi networks

This function makes a REST `GET` call to `https://192.168.35.2/wifi/config/networks`, to list the current Wi-Fi configurations for the attached device.

The Azure Sphere CLI command this API calls is `azsphere device wifi list`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "values": [
    {
      "ssid": "ExampleNetworkName",
      "configState": "enabled",
      "connectionState": "disconnected",
      "id": 0,
      "securityState": "psk",
      "targetedScan": false
    }
  ]
}
```

### Get configured Wi-Fi network

This function makes a REST `GET` call to `https://192.168.35.2/wifi/config/networks/{id}`, to get details of a Wi-Fi network on the attached device. `{id}` is the ID of the network to show details for.

The Azure Sphere CLI command this API calls is `azsphere device wifi show`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "ssid": "ExampleNetworkName",
  "configState": "enabled",
  "connectionState": "disconnected",
  "id": 0,
  "securityState": "psk",
  "targetedScan": false
}
```

### Get Wi-Fi interface state

This function makes a REST `GET` request to `https://192.168.35.2/wifi/interface`, to get the status of the wireless interface on the attached device.

The Azure Sphere CLI command this API calls is `azsphere device wifi show-status`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "ssid": "ExampleNetworkName",
  "configState": "enabled",
  "connectionState": "connected",
  "securityState": "psk",
  "mode": "station",
  "key_mgmt": "WPA2-PSK",
  "wpa_state": "COMPLETED",
  "address": "1c:f1:f1:11:91:a1",
  "freq": 5220,
  "ip_address": "902.21.11.1",
  "id": 0
}
```

#### Get Wi-Fi scan results

This function makes a REST `GET` request to `https://192.168.35.2/wifi/scan`, to scan for available Wi-Fi networks visible to the attached device.

The Azure Sphere CLI command this API calls is `azsphere device wifi scan`.

#### Example response

The following JSON is the example response for a successful HTTP/GET request.

```JSON
{
  "values": [
    {
      "bssid": "11:11:11:11:11:12",
      "freq": 5220,
      "signal_level": -40,
      "ssid": "ExampleNetworkName",
      "securityState": "psk"
    },
    {
      "bssid": "11:11:11:11:11:11",
      "freq": 5220,
      "signal_level": -61,
      "ssid": "I-WPA",
      "securityState": "eaptls"
    },
    {
      "bssid": "11:11:11:11:11:21",
      "freq": 5220,
      "signal_level": -60,
      "ssid": "\u0000",
      "securityState": "psk"
      }
  ]
}
```

### Remove configured Wi-Fi network

This function makes a REST `DELETE` call to `https://192.168.35.2/wifi/config/networks/{id}`, to forget a Wi-Fi network on the attached device. `{id}` is ID of the network to forget.

The Azure Sphere CLI command this API calls is `azsphere device wifi forget`.

#### Example response

The following JSON is the example response for a successful HTTP/DELETE request.

```JSON
{}
```
