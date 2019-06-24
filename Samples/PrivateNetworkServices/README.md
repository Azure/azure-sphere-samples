# Sample: Private Network Services

This sample C application demonstrates how you can [connect an Azure Sphere device to a private network](https://docs.microsoft.com/azure-sphere/network/connect-ethernet) and [use network services](https://docs.microsoft.com/azure-sphere/network/use-network-services). It configures the Azure Sphere device to run a DHCP server and an SNTP server, and implements a basic TCP server. The steps below show how to verify this functionality by connecting your computer to this private network.

The DHCP and SNTP servers are managed by the Azure Sphere OS and configured by the high-level application. The servers start only upon request from the application but continue to run even after the application stops.

The TCP server runs in the application process and stops when the application stops. Note that this sample TCP server implementation is basic, for illustration only, and that it does not authenticate or encrypt connections; you should replace it with your own production logic.

The sample uses the following Azure Sphere libraries and includes [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta).

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|networking    | Gets and sets network interface configuration |

## Prerequisites

 This sample will run on any supported network interface. However, it is configured by default for a private Ethernet network. If using Ethernet, before you build and run this sample you must connect and configure an Ethernet adapter to your MT3620 development board. See how to [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-private-network) and if using an MT3620 RDB, see [add an Ethernet adapter to your development board](../../Hardware/mt3620_rdb/EthernetWiring.md).

To run the sample on a different network interface, modify the code that defines the global constant ``NetworkInterface`` which is found in the source file ``\PrivateNetworkServices\PrivateNetworkServices\main.c``. For example, to specify a Wi-Fi network, change the line
```c
     static const char NetworkInterface[] = "eth0"; 
```

to
```c
     static const char NetworkInterface[] = "wlan0";
```

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.05 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device prep-debug`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the PrivateNetworkServices  sample.
1. In Visual Studio, open PrivateNetworkServices.sln and press F5 to compile and build the solution and load it onto the device for debugging.
1. Leave the app running while you complete the remaining instructions.

### Troubleshooting the Azure Sphere app

- Visual Studio returns the following error if the application fails to compile:

   `1>C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\Application Type\Linux\1.0\AzureSphere.targets(105,5): error MSB6006: "arm-poky-linux-musleabi-gcc.exe" exited with code 1.`

   This error may occur for many reasons. Most often, the reason is that you did not clone the entire Azure Sphere Samples repository from GitHub. The samples depend on the hardware definition files that are supplied in the Hardware folder of the repository.

### To get detailed error information

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

## Connect your computer to the private network
Connect your computer to the same private network to which you connected your device. If using an Ethernet private network, attach an Ethernet cable from the Ethernet adapter on the device to the Ethernet connection on your computer.


**Note:** If your computer is managed by policies that prevent it from being connected to multiple network interfaces at once, you may need to disable other network interfaces while using this sample.

**Note:** The samples uses the IP address range 192.168.100.xxx. If you have another network adapter that uses the same range, you will need to either modify the sample or disable the other network adapter temporarily.

## Test the device's DHCP server

Open a command prompt on your computer and type **ipconfig**. You should see that the DHCP server has issued an IP address in the 192.168.100.xxx range to your PC for its network connection:

```sh
<network interface type> adapter <name>:

   Connection-specific DNS Suffix  . :
   Link-local IPv6 Address . . . . . : fe80::8c67:be24:4d9a:d4bb%9
   IPv4 Address. . . . . . . . . . . : 192.168.100.11
   Subnet Mask . . . . . . . . . . . : 255.255.255.0
   Default Gateway . . . . . . . . . :
```

You could also find, download, and use a DHCP client test tool (not provided) on your PC to inspect the DHCP server response in more detail &mdash; such as to look at the NTP server address(es) returned.

## Test the device's SNTP server

1. Ensure the Azure Sphere device is connected to the internet via a different network interface (for example, Wi-Fi if using private Ethernet), so that it can obtain time settings from a public NTP server. The SNTP server won't respond until it knows the current time.
1. Open a command prompt on your computer and type **w32tm /stripchart /computer:192.168.100.10 /dataonly /samples:1**. This invokes the [Windows Time tool](https://docs.microsoft.com/windows-server/networking/windows-time-service/windows-time-service-tools-and-settings) to query the device's SNTP server and to display the calculated difference between your computer's time and the device's time:
   ```sh
   Tracking 192.168.100.10 [192.168.100.10:123].
   Collecting 1 samples.
   The current time is 06/02/2019 14:18:09.
   14:18:09, +00.0349344s
   ```

1. If the SNTP server doesn't respond, then you may see the following output. Check that the app is running and that the Azure Sphere device is connected to the internet.
   ```sh
   Tracking 192.168.100.10 [192.168.100.10:123].
   Collecting 1 samples.
   The current time is 06/02/2019 14:16:50.
   14:16:50, error: 0x800705B4
   ```

## Test the application's TCP server

Ensure that the sample app is still running on your Azure Sphere device. Then, on your computer, use a terminal application to open a raw TCP connection to the Azure Sphere application's TCP server at 192.168.100.10 port 11000. You can open this connection with a third-party terminal application such as PuTTY (using a "raw" connection type), or with the built-in Telnet client for Windows.

To use the built-in Telnet client for Windows:

1. Open Control Panel and click **Programs and Features** > **Turn Windows features on or off** to launch the **Windows Features** window.
1. Ensure **Telnet Client** is selected and click **OK**.
1. Open a command prompt and type **telnet 192.168.100.10 11000**.

The characters that you type will appear in the debug console in Visual Studio&mdash;either immediately or when you enter a newline&mdash;showing that they have been received by the example TCP server on the MT3620. When you enter a newline, the MT3620 sends the following string back to the terminal:

   ```sh
   Received "<last-received-line>"
   ```

This sample server has a simple 16-character input buffer. If you send more data, the Output window in Visual Studio may show: "Input data overflow. Discarding 16 characters." 


## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).