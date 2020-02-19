
# Sample: Inter-core communication

This sample demonstrates how to exchange messages between applications running on the high-level and real-time capable cores.

Once per second the high-level application sends a message "Hello-World-%d", where %d is an incrementing counter to the real-time capable application. The real-time capable application prints the received data, converts any upper-case characters to lower-case and vice versa, and echoes the message back to the high-level application.

The high-level application uses the following Azure Sphere libraries and includes [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta):

|Library   |Purpose  |
|---------|---------|
|application.h |Communicates with and controls real-time capable applications |
|log.h |Displays messages in the Visual Studio Device Output window during debugging |
|eventloop.h |Invoke handlers for timer events |

The real-time capable features used in the sample are in Beta.

To use this sample, clone the repository locally if you haven't already done so:

```
git clone https://github.com/Azure/azure-sphere-samples.git
```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time capable core UART to a USB port on your PC.
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/.)) to display the output.

## Prep your device

To prep your device on Windows:

1. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More>Run as administrator**. 

   The `--EnableRTCoreDebugging` parameter requires administrator privilege because it installs USB drivers for the debugger.

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

1. Close the window after the command completes because administrator privilege is no longer required. As a best practice, you should always use the lowest privilege that can accomplish a task.

To prep your device on Linux:

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

## Set up hardware to display output

To prepare your hardware to display output from the sample, see [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) for Windows or [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output) for Linux.

## Build and run the apps

The applications in this sample run as partners. Make sure that they're designated as partners as described in [sideload more than one application](https://docs.microsoft.com/azure-sphere/app-development/sideload-app#sideload-more-than-one-application) so that sideloading one doesn't delete the other.

### Build and run the RTApp

See the following Azure Sphere Quickstarts to learn how to build, and deploy a real-time application:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application)
   -  [with VS Code on Windows or Linux](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-vscode)
   -  [on the Windows or Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-cli)

### Build and run the high level application

See the following Azure Sphere Quickstarts to learn how to build and deploy a high-level application:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

## Observe the output

The high-level application output will be displayed in the Output window in Visual Studio.

```sh
Remote debugging from host 192.168.35.1
High-level intercore application.
Sends data to, and receives data from the real-time core.
Sending: Hello-World-0
Received 13 bytes: hELLO-wORLD-0
Sending: Hello-World-1
Received 13 bytes: hELLO-wORLD-1
Sending: Hello-World-2
```

The real-time core application output will be sent to the serial terminal for display.

```sh
IntercoreComms_RTApp_MT3620_BareMetal
App built on: Dec 12 2019, 10:17:38
Received message of 33 bytes:
  Component ID (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:30
  Payload (13 bytes as text): Hello-World-0
Received message of 33 bytes:
  Component ID (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:31
  Payload (13 bytes as text): Hello-World-1
Received message of 33 bytes:
  Component ID (16 bytes): 25025d2c-66da-4448-bae1-ac26fcdd3627
  Reserved (4 bytes): 00280003
  Payload (13 bytes as hex): 48:65:6c:6c:6f:2d:57:6f:72:6c:64:2d:32
  Payload (13 bytes as text): Hello-World-2
```
