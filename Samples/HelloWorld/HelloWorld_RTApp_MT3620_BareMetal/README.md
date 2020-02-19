# Sample: MT3620 real-time capability application - Hello World

This sample demonstrates how to create, deploy, and run [MT3620 real-time capable applications](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application).

This sample app for an MT3620 real-time core repeatedly transmits a simple message over a UART. These messages can be read in terminal application on a computer using a USB-to-serial adapter. By default, it uses the real-time core's dedicated UART, but if your hardware doesn't expose this UART's TX pin, then the sample can be altered to use a different UART.

To use this sample, clone the repository locally if you haven't already done so:

  ```shell
  git clone https://github.com/Azure/azure-sphere-samples.git
  ```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A breakout board and USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time core UART to a USB port on your computer. 
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) to display the output.

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

## Build and run the sample

See the following Azure Sphere Quickstarts to learn how to build and deploy this sample:
  
   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application)
   -  [with VS Code on Windows or Linux](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-vscode)
   -  [on the Windows or Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-cli)

## Observe the output

The connected terminal emulator should display output from the HelloWorld_RTApp_MT3620_Baremetal program. The program sends the following words at one-second intervals:

   `Tick`  
   `Tock`
