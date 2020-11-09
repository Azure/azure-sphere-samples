# Sample: CustomNTP_HighLevelApp

This sample application demonstrates how to configure custom NTP servers on an MT3620 device. After you configure the sample with your NTP server configuration in the application manifest, you can use the button and status LED on the device as follows:

SAMPLE_BUTTON_1 does the following:
- Gets the last time synced information
   - If the device has not yet successfully time synced, the sample logs a debug message informing the user that the device has not time synced as yet.
   - If the device has successfully time synced, the sample logs a message displaying the time before the sync, and the new adjusted time after the successful sync.

Status LED indicates the following:
- Red - Not time synced. This state is true on reboot till the device successfully time syncs with the NTP server, or when the NTP server config is changed and the device has not yet synced with the NTP server.
- Green - The device has time synced successfully to the NTP server. This state is true when the device successfully syncs with the configured NTP server.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview)     |  Manages buttons A and B on the device |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview)     |  Displays messages in the Device Output window during debugging  |
| [networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Networking related API calls |

## Contents

| File/Folder | Description |
|-------------|-------------|
| main.c    | Sample source file. |
| app_manifest.json | Sample manifest file. |
| CMakeLists.txt | Contains the project information and produces the build. |
| CMakeSettings.json| Configures Visual Studio to use CMake with the correct command-line options. |
| launch.vs.json | Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
| .vscode | Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

## Prerequisites

The sample requires the following hardware:

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](../../../HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. In CustomNTP_HighLevelApp/main.c, ensure that the global constant `networkInterface` is set to "eth0". In the source file CustomNTP_HighLevelApp/main.c, search for the following line:

    `static const char networkInterface[] = "wlan0";`

    Change this line to:

    `static const char networkInterface[] = "eth0";`
1. In CustomNTP_HighLevelApp/main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    int err = Networking_SetInterfaceState(networkInterface, true);
    if (err == -1) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the CustomNTP_HighLevelApp sample in the CustomNTP folder.

## Configure the sample application to work with your NTP server configuration
There are three different types of NTP server configurations possible:

1. System default (prod.time.sphere.azure.net)
1. DHCP (Automatic) NTP server
1. Custom NTP server

The sample can be configured with any one type at a time.

For more information, see [Specifying an NTP Server](https://docs.microsoft.com/azure-sphere/app-development/rtc#Specifying-an-NTP-Server).

### System Default
This uses the system default (prod.time.sphere.azure.net) as the time source. 
To configure the sample to connect to the system default NTP server, copy and paste the following line into the CmdArgs field of the app_manifest.json file:

`"CmdArgs": [ "--TimeSource", "Default" ]`

### Automatic NTP server
This configures the sample to connect to the NTP servers assigned by DHCP. 
You will need the following information:
- The time source to use
- Fallback enabled or disabled

Update the CmdArgs field of the app_manifest.json file:
- To configure the sample to connect to the DHCP assigned NTP servers, copy and paste the following line into the CmdArgs field of the app_manifest.json file:

   `"--TimeSource", "Automatic"`
- Fallback is enabled by default. Configure this option only if you want fallback to be disabled.
  To disable fallback, set the `DisableFallback` option in the CmdArgs field of the app_manifest.json file, as shown below:

   `"--DisbleFallback"`

   Note: This option does not have an argument.

- Your CmdArgs field should now look like:
   - With fallback enabled

   `"CmdArgs": [ "--TimeSource", "Automatic" ]`

   - With fallback disabled

   `"CmdArgs": [ "--TimeSource", "Automatic", "--DisableFallback" ]`


### Custom NTP servers
This configures the sample to connect to up to two user-configured NTP servers. If you do not have access to an NTP server for testing, see https://www.pool.ntp.org/  for a list of NTP servers you could use with the sample.

You will need the following information:
- The time source to use
- Fallback enabled or disabled
- Primary NTP server hostname or IP.
- Secondary NTP server hostname or IP. This is optional.

Update the CmdArgs field of the app_manifest.json file:
- To configure the sample to connect to the DHCP assigned NTP servers, copy and paste the following line into the CmdArgs field of the app_manifest.json file:

   `"--TimeSource", "Custom"`

- Fallback is enabled by default. Configure this option only if you want fallback to be disabled.
  To disable fallback, set the `DisableFallback` option in the CmdArgs field of the app_manifest.json file, as shown below:

   `"--DisableFallback"`

   Note: This option does not have an argument.

- Specify the hostname or IP of the primary NTP server in the CmdArgs field of the app_manifest.json file, as shown below:

   `"--PrimaryNTPServer", "<hostname_or_ip>"`

- Optionally, you can also specify the hostname or IP of the secondary NTP server in the CmdArgs field of the app_manifest.json file, as shown below:

   `"--SecondaryNTPServer", "<hostname_or_ip>"`

- Your CmdArgs field should now look like:
   - Without secondary NTP server configured and fallback enabled
   
      `"CmdArgs": [ "--TimeSource", "Custom", "--PrimaryNtpServer", "<hostname_or_ip>" ]`

   - Without secondary NTP server configured and fallback disabled
   
      `"CmdArgs": [ "--TimeSource", "Custom", "--DisableFallback", "--PrimaryNtpServer", "<hostname_or_ip>" ]`

   - With secondary NTP server configured and fallback enabled

      `"CmdArgs": [ "--TimeSource", "Custom", "--PrimaryNtpServer", "<hostname_or_ip>", "--SecondaryNtpServer", "<hostname_or_ip>" ]`

   - With secondary NTP server configured and fallback disabled

      `"CmdArgs": [ "--TimeSource", "Custom", "--DisableFallback", "--PrimaryNtpServer", "<hostname_or_ip>", "--SecondaryNtpServer", "<hostname_or_ip>" ]`


## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../..//BUILD_INSTRUCTIONS.md).

## Test the sample
The output will be displayed in the terminal window. When the sample runs, it will configure the NTP servers as per your configuration in the application manifest file.

Use SAMPLE_BUTTON_1 and Status LED as directed in the sample description, above.