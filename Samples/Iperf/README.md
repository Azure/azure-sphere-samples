# Tool: iPerf
iPerf is a tool for active measurements of the maximum achievable bandwidth on IP networks. It can be used to diagnose network issues, or to generate network traffic for manufacturing/certification tests. This project provides a __client only__ implementation of `iperf3` compatible with azure sphere. The project leverages the [open source implementation of iPerf](https://github.com/esnet/iperf) at version 3.11. For more information about `iperf3` [visit GitHub](https://github.com/esnet/iperf) or explore the documentation contained in the `iperf` submodule once initialized.

At a high-level, `iperf3` on Azure Sphere acts as a client to an `iperf3` server running on another device. Various measurements are taken between client and server which can be used to detect and diagnose network issues. An iPerf server can be run on a machine attached to a local network, or a machine hosted elsewhere connected to the internet. It is therefore imperative that the Azure Sphere `iperf3` client device is connected to a network with corresponding access (local, or internet).

This project adopts the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

## Contents

| File/folder         | Description |
|---------------------|-------------|
| `app_manifest.json`   | The application manifest file that describes the resources required. |
| `CMakeLists.txt`      | A CMake configuration file that contains the project information required for all builds. |
| `CMakePresets.json`   | A CMake presets file that contains the CMake configuration for project. |
| `launch.vs.json`      | A JSON file that informs Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `iperf`               | A git submodule that refers to [esnet/iperf](https://github.com/esnet/iperf). The submodule must be intiialized before building the application. |
| `overrides`           | A folder that contains the modifications required to allow Azure Sphere to operate as an `iperf `client. |
| `README.md`           | This README file. |
| `.vscode`             | A folder containing the JSON files that configure Visual Studio Code for deploying and debugging the application. |

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits).

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../HardwareDefinitions/README.md) file.

## Setup

1. Ensure that your Azure Sphere device is connected to your computer and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.11 or later. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and navigate to `/Samples/Iperf`.
1. Initialize the iperf submodule by running: `git submodule init`, `git submodule sync`, and `git submodule update`.
1. Configure networking on your device. You must either [set up WiFi](https://learn.microsoft.com/azure-sphere/install/configure-wifi#set-up-wi-fi-on-your-azure-sphere-device) or [set up Ethernet](https://learn.microsoft.com/azure-sphere/network/connect-ethernet) on your development board, depending on the type of network connection you are using.

## Running `iperf3` on Azure Sphere

`iperf3` on Azure Sphere is client only and requires a server to measure against. The server can be [hosted on a local machine](#run-an-iperf-server-on-windows) or you can make use of [a public iperf server](https://iperf.fr/iperf-servers.php). **Note: public server use is discouraged due to resource contention with other users.**

Before deploying the application, remember to update the `app_manifest.json` file . Replace each instance of `<IPERF SERVER IP ADDRESS>` and `<IPERF SERVER PORT>` with the corresponding `iperf3` server ip address and port. The IP address of a local machine can be obtained through `ifconfig` on Unix-based systems, and `ipconfig` on Windows. **Remember to use the network interface ip address the Azure Sphere device is connected to. Do not use "Ethernet adapter Azure Sphere".**

```json
"CmdArgs": [ "-c","<IPERF SERVER IP ADDRESS>","-t","60","-p","<IPERF SERVER PORT>","-l","8192","-w","8192"],
```

Ensure that you also add the iPerf server IP address to `AllowedConnections` under `Capabilities` in the `app_manifest.json` file:

```json
"Capabilities": {
   "AllowedConnections": ["<IPERF SERVER IP ADDRESS>"]
},
```

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

With the correct environment set up, `iperf3` logs will be displayed in the 'Output' pane of VS Code or Visual Studio. If running an `iperf3` server on a local machine, output will also be displayed on the terminal in which the server was instantiated.

### Additional configuration options

`iperf3` takes a number of command line parameters. Command line parameters can be configured via the `app_manifest.json` file. All possible command line parameters are listed on the [`iperf3` manual page](https://software.es.net/iperf/invoking.html#iperf3-manual-page).

Please do not remove the following compulsory command line parameters:

```json
"-l", "8192", "-w", "8192"
```
The compulsory parameters above keep the `iperf3` application within the memory bounds of an Azure Sphere device.

## Run an iperf server on Windows

1. Download a zip archive containing pre-built Windows binary from [iperf.fr](https://iperf.fr) or [GitHub](https://github.com/ar51an/iperf3-win-builds).
1. Extract the zip archive and open a terminal at the unzipped folder location.
1. Invoke `iperf3.exe` by running `iperf3.exe -s`. Windows may open a firewall prompt. Allow the `iperf` through the firewall. By default, the `iperf3` server can be accessed via port 5201. To set an alternate port, use the `-p` flag: `iperf3.exe -s -p 5044`.

### Run an iperf server on Linux, Mac or WSL2
1. [Download or clone](https://github.com/esnet/iperf) the source tree for `iperf`.
1. Navigate to the cloned repository and run `./configure && make && make install`.
1. Once installation is complete, `iperf3` should be available on the path. Create an `iperf3` server by running: `iperf3 -s`. By default, the `iperf3` server can be accessed via port 5201. To set an alternate port, use the `-p` flag: `iperf3 -s -p 5044`.

#### Linux, Mac, WSL2 troubleshooting
* libiperf.so.0: cannot open shared object file: No such file or directory

   ```
   iperf3 -s
   iperf3: error while loading shared libraries: libiperf.so.0: cannot open shared object file: No such file or directory
   ```
   As discussed in [this GitHub issue](https://github.com/esnet/iperf/issues/153), please run `ldconfig`. This may require `sudo`.

* Permission denied when running `make install`

   Run: `sudo make install`
