# Sample: MT3620 real-time capability application - Hello World

This sample demonstrates how to create, deploy, and run [MT3620 real-time capable applications](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application).

This sample app for an MT3620 real-time core repeatedly transmits a simple message over a UART. These messages can be read in terminal application on a computer using a USB-to-serial adapter. By default, it uses the real-time core's dedicated UART, but if your hardware doesn't expose this UART's TX pin, then the sample can be altered to use a different UART.

Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *HelloWorld_RTApp_MT3620_BareMetal* sample in the *HelloWorld* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/helloworld/).

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. A breakout board and USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time core UART to a USB port on your computer. 
1. A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) to display the output.

## Prep your device

To prep your device on Windows:

1. Open the [Azure Sphere command-line tool](https://docs.microsoft.com/azure-sphere/reference/overview) with administrator privileges. 

   Administrator privileges are required for enabling real-time core debugging because it installs USB drivers for the debugger.

1. Enter the [**azsphere device enable-development**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) command:

    Azure Sphere CLI:

    ```
    azsphere device enable-development --enable-rt-core-debugging
    ```

    Azure Sphere classic CLI:

    ```
    azsphere device enable-development --enablertcoredebugging
    ```

1. Close the window after the command completes because administrator privilege is no longer required. As a best practice, you should always use the lowest privilege that can accomplish a task.

To prep your device on Linux:

1. Enter the [**azsphere device enable-development**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) command:

    Azure Sphere CLI:

    ```
    azsphere device enable-development --enable-rt-core-debugging
    ```

    Azure Sphere classic CLI:

    ```
    azsphere device enable-development --enablertcoredebugging
    ```

## Set up hardware to display output

To prepare your hardware to display output from the sample, see [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application#set-up-hardware-to-display-output).

## Build and run the sample

See [Tutorial: Build a real-time capable application](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application) to learn how to build and deploy this sample.

## Test the sample

The connected terminal emulator should display output from the HelloWorld_RTApp_MT3620_Baremetal program. The program sends the following words at one-second intervals:

   `Tick`  
   `Tock`
