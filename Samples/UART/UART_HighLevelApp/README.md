---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – UART
urlFragment: UART
extendedZipContent:
- path: HardwareDefinitions
  target: HardwareDefinitions
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to communicate over the UART on an Azure Sphere device."
---

# Sample: UART high-level app

This sample demonstrates how to communicate over the UART on an MT3620 development board.

This sample does the following:

- Opens a UART serial port with a baud rate of 115200.
- Sends characters from the device over the UART when button A is pressed.
- Displays the data received from the UART in the Output Window of Visual Studio or Visual Studio Code.
- Causes an LED to blink when data is received from the UART.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for I/O and timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |
| [uart](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-uart/uart-overview) | Manages UART connectivity on the device. |

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

## Prerequisites

This sample requires the following hardware:

- An Azure Sphere MT3620 board

   **Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md). You may also have to set the hardware differently.

- Make a loopback connection on header 2 (marked H2) on the lower left side of the board by connecting pins 1 and 3 (ISU0 RXD and ISU0 TXD) of H2 with a jumper. Pins 1 and 3 are the first two pins on the left side of the header, circled in red in the image.

   ![RDB with header pins circled](./media/MT3620UartJumper.png) 

- As an alternative to using the loopback connection, you can connect the UART to an external serial-USB interface board and use a client such as Telnet or Putty to transmit and receive bytes. This solution has been tested using the Adafruit FTDI Friend serial-to-USB adapter with the following wiring connections.

   ![Connections for MT3620 and FTDI Friend](./media/MT3620_FTDI-Friend-2.png)

   | FTDI Friend | Azure Sphere MT3620 |
   | ----------- | ------------------- |
   | GND         | H2 Pin 2            |
   | TX          | H2 Pin 1            |
   | RX          | H2 Pin 3            |
   | RTS         | H2 Pin 5            |
   | CTS         | H2 Pin 7            |

  **Putty settings:**

   - Local echo = force on
   - Local line editing = force on

## Setup

1. Even if you've performed this setup previously, ensure you have Azure Sphere SDK version 21.04 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *UART_HighLevelApp* sample in the *UART* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/uart/).

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

Press button A on the board. This sends 13 bytes over the UART connection and displays the sent and received text in the **Device Output** window, if you're using Visual Studio or Visual Studio Code:

`Sent 13 bytes over UART in 1 calls.`  
`UART received 12 bytes: 'Hello world!'.`  
`UART received 1 bytes: '`  
`'.`

The received text might not appear all at the same time and it might not appear immediately.

The message may contain more bytes than the **read** function can return, depending on the Azure Sphere device (on the MT3620 this is often 12 bytes). The message may need to be assembled asynchronously over a sequence of **read** calls as a result, as illustrated at [this point](https://github.com/Azure/azure-sphere-samples/blob/7232fcb52a493b7def65c50ea93ab9bb73e283c2/Samples/WifiSetupAndDeviceControlViaBle/AzureSphereApp/WifiSetupAndDeviceControlViaBle/message_protocol.c#L214) in the WifiSetupAndDeviceControlViaBle sample.

If it is temporarily not possible to send further bytes, such as when transmitting larger buffers, the **write** function may fail with errno of **EAGAIN**. You can handle this by registering an **EPOLLOUT** event handler, as illustrated at [this point](https://github.com/Azure/azure-sphere-samples/blob/7232fcb52a493b7def65c50ea93ab9bb73e283c2/Samples/WifiSetupAndDeviceControlViaBle/AzureSphereApp/WifiSetupAndDeviceControlViaBle/message_protocol.c#L276) in the WifiSetupAndDeviceControlViaBle sample.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- For information about how UARTs can be used in high-level applications, see [Use UARTs in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/uart).
