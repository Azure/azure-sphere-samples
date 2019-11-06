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

## To prepare the sample

1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 19.10 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK Preview](https://docs.microsoft.com/azure-sphere/install/install-sdk) for Visual Studio or Windows as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the DNSServiceDiscovery sample in the DNSServiceDiscovery folder.

## To build and run the sample

### Building and running the sample with Visual Studio

1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**.

1. Go to the **Build** menu, and select **Build All**. Alternatively, open **Solution Explorer**, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.

1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

### Building and running the sample from the Windows CLI

Visual Studio is not required to build an Azure Sphere application. You can also build Azure Sphere applications from the Windows command line. To learn how, see [Quickstart: Build the Hello World sample application on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli). It walks you through an example showing how to build, run, and prepare for debugging an Azure Sphere sample application.

## Testing the service connection

When you run the application, it displays the name, host, IPv4 address, port, and TXT data from the query response. The application should then be able to connect to the host names returned by the response.

You can verify the connection by setting up a local web server on the same computer as the DNS service, and then making requests to the service from the application.

To set up a web server using IIS:

1. Install [IIS](https://www.iis.net/) on the same computer as the DNS service.
1. If you set up a site binding for a default website with a port other than 80 or 443, you must add an inbound rule that allows the port.

To send requests to the web server, you can incorporate code from the [HTTPS_Curl_Easy](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/HTTPS/HTTPS_Curl_Easy) sample into the application. Requests to the web server should fail before the DNS-SD responses are received but should succeed afterwards.

## To specify another DNS service

By default, this sample queries the _sample-service._tcp.local DNS server address. To query a different DNS server, make the following changes:

1. Open app_manifest.json.
1. Change the value of the `AllowedConnections` field from `"_sample-service._tcp.local"` to the new DNS server address, such as `“_http._tcp.local”`.
1. Open main.c.
1. Go to the line `static const char DnsServiceDiscoveryServer[] = "_sample-service._tcp.local";"` and replace `_sample-service._tcp.local` with the new DNS server address.

## Sending unicast queries

If you don't need to use multicast queries, you can use unicast queries by calling the res_send() POSIX API to query the DNS server and process the response in a single blocking call. This may simplify the application, especially if it doesn’t need to perform other activities while waiting for the response.
