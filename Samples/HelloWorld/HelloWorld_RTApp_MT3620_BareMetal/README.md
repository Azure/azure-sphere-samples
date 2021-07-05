# Sample: MT3620 real-time capability application - Hello World

This sample demonstrates how to create, deploy, and run [MT3620 real-time capable applications](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application).

This sample app for an MT3620 real-time core repeatedly transmits a simple message over a UART. These messages can be read in terminal application on a computer using a USB-to-serial adapter. By default, it uses the real-time core's dedicated UART, but if your hardware doesn't expose this UART's TX pin, then the sample can be altered to use a different UART.

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

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits).
- A breakout board and USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time core UART to a USB port on your computer.
- A terminal emulator, such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/), to display the output.

## Setup

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *HelloWorld_RTApp_MT3620_BareMetal* sample in the *HelloWorld* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/helloworld/).

1. Prepare your device on Windows or Linux:

   **On Windows:**

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

   **On Linux:**

   1. Enter the [**azsphere device enable-development**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) command:

       Azure Sphere CLI:

       ```
       azsphere device enable-development --enable-rt-core-debugging
       ```

       Azure Sphere classic CLI:

       ```
       azsphere device enable-development --enablertcoredebugging
       ```

1. Prepare your hardware to display output from the sample. See [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application#set-up-hardware-to-display-output) for instructions.

## Build and run the sample

See [Tutorial: Build a real-time capable application](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application) to learn how to build and deploy this sample.

### Observe the output

The connected terminal emulator should display output from the HelloWorld_RTApp_MT3620_Baremetal program. The program sends the following words at one-second intervals:

   `Tick`  
   `Tock`
