---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Inter-core communication
urlFragment: IntercoreComms
extendedZipContent:
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to exchange messages between applications running on the high-level and real-time cores."
---

# Sample: Inter-core communication

This sample demonstrates how to exchange messages between applications running on the high-level and real-time cores.

**Note:** Before you run this sample, see [Communicate with a high-level application](https://docs.microsoft.com/azure-sphere/app-development/inter-app-communication). It describes how real-time capable applications communicate with high-level applications on the MT3620.

Once per second the high-level application (HLApp) sends a message "hl-app-to-rt-app-%d", where %d cycles between 00 and 99. The real-time capable application (RTApp) prints the received message. Once per second the RTApp sends a message "rt-app-to-hl-app-%d" to the HLApp, where %d cycles between 00 and 99. The HLApp prints the received message.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [application](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Communicates with and controls the real-time capable application. |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |

**Note:** These libraries are used only by the high-level application.

## Contents

| File/folder                             | Description |
|-----------------------------------------|-------------|
| `intercore.code-workspace`              | A Visual Studio Code workspace file that allows building and debugging the RTApp and the high-level app at the same time. |
| `launch.vs.json`                        | JSON file that tells Visual Studio how to deploy and debug the application. |
| `README.md`                             | This README file. |
| `IntercoreComms_HighLevelApp`           | Folder containing the configuration files, source code files, and other files needed for the high-level application. |
| `IntercoreComms_RTApp_MT3620_BareMetal` | Folder containing the configuration files, source code files, and other files needed for the real-time capable application (RTApp). |

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits).

- A USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time capable core UART to a USB port on your PC.
- A terminal emulator, such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/), to display the output.

## Setup

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *IntercoreComms* sample in the *IntercoreComms* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/intercorecomms/).

1. Prepare your device on Windows or Linux.

   **To prepare your device on Windows:**

   1. Open the [Azure Sphere command-line tool](https://docs.microsoft.com/azure-sphere/reference/overview) with administrator privileges. 

      Administrator privileges are required for enabling real-time core debugging because it installs USB drivers for the debugger.

   1. Enter the [**azsphere device enable-development**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) command as follows:  

       Azure Sphere CLI:

       ```
       azsphere device enable-development --enable-rt-core-debugging
       ```

       Azure Sphere classic CLI:

       ```
       azsphere device enable-development --enablertcoredebugging
       ```

   1. Close the window after the command completes because administrator privilege is no longer required. As a best practice, you should always use the lowest privilege that can accomplish a task.

   **To prepare your device on Linux:**

   1. Enter the [**azsphere device enable-development**](https://docs.microsoft.com/azure-sphere/reference/azsphere-device#enable-development) command as follows:  

       Azure Sphere CLI:

       ```
       azsphere device enable-development --enable-rt-core-debugging
       ```

       Azure Sphere classic CLI:

       ```
       azsphere device enable-development --enablertcoredebugging
       ```

1. Set up the hardware to display output from the RTApp. For instructions, see [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application#set-up-hardware-to-display-output).

## Build and run the sample

The applications in this sample run as partners. Make sure that they're designated as partners, as described in [Mark applications as partners](https://docs.microsoft.com/azure-sphere/app-development/sideload-app#mark-applications-as-partners), so that sideloading one doesn't delete the other.

If you're using Visual Studio or Visual Studio Code, you will need to deploy and debug both apps simultaneously. See the following instructions for building and running
the sample with Visual Studio or Visual Studio Code.

**To build and run the sample with Visual Studio:**

1. On the **File** menu, select **Open > Folder**.
1. Navigate to your Azure Sphere samples directory, select IntercoreComms, and click **Select Folder**.
1. On the **Select Startup Item** menu, select **GDB Debugger (All Cores)**.
1. On the **Build** menu, select **Build All**.
1. On the **Debug** menu, select **Start**, or press **F5**.

**To build and run the sample with Visual Studio Code:**

1. Use the [Visual Studio Code Multi-root Workspaces](https://code.visualstudio.com/docs/editor/multi-root-workspaces) feature to build and debug the RTApp and high-level app at the same time.
1. On the **File** menu, **Select Open Workspace**.
1. Navigate to the IntercoreComms root directory and select the file *intercore.code-workspace*. 
1. Click **Open**.
1. After the build files have been created, right-click on either of the two *CMakeLists.txt* files and select **Build All Projects**.
1. Click the **Run** icon in the menu on the left side of the screen.
1. On the pulldown menu, that appears at the top of the window on the left side of the screen, select **Launch for azure Sphere Applications (gdb)(workspace)**.
1. On the **Run** menu, select **Start Debugging**. 

If you're running the sample from the command line you will need to build and run the RTApp before you build and run the high-level app. For more information about building real-time capable and high-level applications from the command line, go to [Build a sample application](../../BUILD_INSTRUCTIONS.md) and click on the links *Tutorial: Build a real-time capable application* and *Build and run a high-level sample with the CLI* respectively.

### Test the sample

The high-level application output will be displayed in the Output window in Visual Studio or Visual Studio Code.

```sh
Remote debugging from host 192.168.35.1, port 55990
High-level intercore comms application.
Sends data to, and receives data from a real-time capable application.
Sending: hl-app-to-rt-app-00
Received 19 bytes: rt-app-to-hl-app-01
Sending: hl-app-to-rt-app-01
Received 19 bytes: rt-app-to-hl-app-01
Sending: hl-app-to-rt-app-02
Received 19 bytes: rt-app-to-hl-app-01
```

Because the HLApp and RTApp are not synchronized, the specific numbers in the messages may start from different places.

The real-time capable application output will be sent to the serial terminal for display.

```sh
--------------------------------
IntercoreComms_RTApp_MT3620_BareMetal
App built on: Mar 21 2020, 13:23:18
Sender: 25025d2c-66da-4448-bae1-ac26fcdd3627
Message size: 19 bytes:
Hex: 68:6c:2d:61:70:70:2d:74:6f:2d:72:74:2d:61:70:70:2d:30:30
Text: hl-app-to-rt-app-00
Sender: 25025d2c-66da-4448-bae1-ac26fcdd3627
Message size: 19 bytes:
Hex: 68:6c:2d:61:70:70:2d:74:6f:2d:72:74:2d:61:70:70:2d:30:31
Text: hl-app-to-rt-app-01
```

Again, the numbers in the messages may start from different places.

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn about partner-application development, see [Create partner applications](https://docs.microsoft.com/azure-sphere/app-development/create-partner-apps).
- To learn about how a high-level application communicates with an RTApp, see [Communicate with a real-time capable application](https://docs.microsoft.com/azure-sphere/app-development/high-level-inter-app).
