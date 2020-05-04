# Sample: DNS service discovery

**Note:** DNS-SD is currently a Beta OS feature.

This sample demonstrates how to perform [DNS service discovery](https://docs.microsoft.com/azure-sphere/app-development/service-discovery) by sending DNS-SD queries to the local network using multicast DNS (mDNS).

The application queries the local network for **PTR** records that identify all instances of the _sample-service._tcp service. The application then queries the network for the **SRV**, **TXT**, and **A** records that contain the DNS details for each service instance.

After service discovery is performed, the Azure Sphere firewall allows the application to connect to the discovered host names.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages network connectivity |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/en-gb/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
|   main.c    | Sample source file. |
| app_manifest.json |Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures Visual Studio to use CMake with the correct command-line options. |
|launch.vs.json |Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode |Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

The sample requires the following hardware:

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the Hardware folder](../../../Hardware/README.md).

## Prepare the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.04 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable application development, if you have not already done so:

   `azsphere device enable-development`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the DNSServiceDiscovery sample in the DNSServiceDiscovery folder.

### Network configuration

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../Hardware/mt3620_rdb/EthernetWiring.md).
1. Add the following line to the Capabilities section of the app_manifest.json file:
   `"NetworkConfig" : true`
1. In main.c, ensure that the global constant `NetworkInterface` is set to "eth0". In source file DNSServiceDiscovery/main.c, search for the following line:

     `static const char NetworkInterface[] = "wlan0"`;

   and change it to:

     `static const char NetworkInterface[] = "eth0"`;
1. In main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    int err = Networking_SetInterfaceState("eth0", true);
    if (err == -1) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

### DNS service

This sample requires that you run a DNS service instance that is discoverable on the same local network as the Azure Sphere device. You can use the dns-sd tool from [Apple Bonjour](https://developer.apple.com/bonjour/) to set up the service. This dns-sd command registers an instance of a DNS responder service with the default service configuration used by the sample:

```
Dns-sd -R SampleInstanceName _sample-service._tcp local 1234 SampleTxtData
```

The command registers a service instance with the following configuration:

- service instance name: SampleInstanceName
- DNS server type: _sample-service._tcp
- DNS server domain: local
- port: 1234
- TXT record: SampleTxtData

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build and deploy this sample:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

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
