# Sample: DNS service discovery

This sample demonstrates how to perform [DNS service discovery](https://docs.microsoft.com/azure-sphere/application-developement/service-discovery) by sending DNS-SD queries to the local network using multicast DNS (mDNS).

The application queries the local network for **PTR** records that identify all instances of the _sample-service._tcp service. The application then queries the network for the **SRV**, **TXT**, and **A** records that contain the DNS details for each service instance.

After service discovery is performed, the Azure Sphere firewall allows the application to connect to the discovered host names.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages network connectivity |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |

## Prerequisites

The sample requires the following hardware:

- Azure Sphere device

### Network configuration

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../Hardware/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:
   `"NetworkConfig" : true`
1. In main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    err = Networking_SetInterfaceState("eth0", true);
    if (err < 0) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

1. In the Project Properties, ensure that the Target API Set is 3+Beta1909.

### DNS service

This sample requires that you run a DNS service instance that is discoverable on the same local network as the Azure Sphere device. You can use the dns-sd tool from [Apple Bonjour](https://developer.apple.com/bonjour/) to setup the service. This dns-sd command registers an instance of a DNS responder service with the default service configuration used by the sample:

```
Dns-sd -R SampleInstanceName _sample-service._tcp local 1234 SampleTxtData
```

The command registers a service instance with the following configuration:

- service instance name: SampleInstanceName
- DNS server type: _sample-service._tcp
- DNS server domain: local
- port: 1234
- TXT record: SampleTxtData

## To build and run the sample

1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device prep-debug`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the DNSServiceDiscovery sample in the DNSServiceDiscovery folder.
1. In Visual Studio, open DNSServiceDiscovery.sln and press F5 to build and load the application onto the device for debugging.

## Testing the service connection

When you run the application, it displays the name, host, IPv4 address, port, and TXT data from the query response. The application should then be able to connect to the host names returned by the response.

You can verify the connection by setting up a local web server on the same computer as the DNS service, and then making requests to the service from the application.

To set up a web server using IIS:

1. Install [IIS](https://www.iis.net/) on the same computer as the DNS service.
1. If you set up a site binding for a default website with a port other than 80 or 443, you must add an inbound rule that allows the port.

To send requests to the web server, you can incorporate code from the [HTTPS_Curl_Easy](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/HTTPS/HTTPS_Curl_Easy) sample into the application. Requests to the web server should fail before the DNS-SD responses are received but should succeed afterwards.

### Troubleshooting the Azure Sphere app

- Visual Studio returns the following error if the application fails to compile:

   `1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

   This error may occur for many reasons. Most often, the reason is that you did not clone the entire Azure Sphere Samples repository from GitHub. The samples depend on the hardware definition files that are supplied in the Hardware folder of the repository.

#### To get detailed error information

By default, Visual Studio may only open the Error List panel, so that you see error messages like this:

`1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

To get more information, open the Build Output window. To open the window, select **View->Output**, then choose **Build** on the drop-down menu. The Build menu shows additional detail, for example:

```
1>------ Rebuild All started: Project: AzureIoT, Configuration: Debug ARM ------
1>main.c:36:10: fatal error: hw/sample_hardware.h: No such file or directory
1> #include <hw/sample_hardware.h>
1>          ^~~~~~~~~~~~~~~~~~~~~~
1>compilation terminated.
1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.
1>Done building project "AzureIoT.vcxproj" -- FAILED.
========== Rebuild All: 0 succeeded, 1 failed, 0 skipped ==========
```

In this case, the error is that hardware definition files aren't available.

The **Tools -> Options -> Projects and Solutions -> Build and Run** panel provides further controls for build verbosity.

## To specify another DNS service

By default, this sample queries the _sample-service._tcp.local DNS server address. To query a different DNS server, make the following changes:

1. Open app_manifest.json.
1. Change the value of the `AllowedConnections` field from `"_sample-service._tcp.local"` to the new DNS server address, such as `“_http._tcp.local”`.
1. Open main.c.
1. Go to the line `static const char DnsServiceDiscoveryServer[] = "_sample-service._tcp.local";"` and replace `_sample-service._tcp.local` with the new DNS server address.

## Sending unicast queries

If you don't need to use multicast queries, you can use unicast queries by calling the res_send() POSIX API to query the DNS server and process the response in a single blocking call. This may simplify the application, especially if it doesn’t need to perform other activities while waiting for the response.
