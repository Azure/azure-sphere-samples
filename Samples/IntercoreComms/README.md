
# Sample: Inter-core communication

This sample demonstrates how to exchange messages between applications running on the high-level and real-time cores.

**Note:** Before you run this sample, see [Communicate with a high-level application](https://docs.microsoft.com/azure-sphere/app-development/inter-app-communication). It describes how real-time capable applications communicate with high-level applications on the MT3620.

Once per second the high-level application (HLApp) sends a message "hl-app-to-rt-app-%d", where %d cycles between 00 and 99.
The real-time capable application (RTApp) prints the receieved message.
Once per second the RTApp sends a message "rt-app-to-hl-app-%d" to the HLApp, where %d cycles between 00 and 99.
The HLApp prints the received message.

The HLApp uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|[application.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) |Communicates with and controls real-time capable applications |
|[log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) |Displays messages in the Visual Studio Device Output window during debugging |
|[eventloop.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) |Invoke handlers for timer events |

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

   The `--enablertcoredebugging` parameter requires administrator privilege because it installs USB drivers for the debugger.

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

1. Close the window after the command completes because administrator privilege is no longer required. As a best practice, you should always use the lowest privilege that can accomplish a task.

To prep your device on Linux:

1. Enter the following azsphere command:

   `azsphere device enable-development --enablertcoredebugging`

## Set up hardware to display output

To prepare your hardware to display output from the sample, see [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/development-environment-windows#set-up-hardware-to-display-output) for Windows or [Set up hardware to display output](https://docs.microsoft.com/azure-sphere/install/development-environment-linux#set-up-hardware-to-display-output) for Linux.

## Build and run the apps

The applications in this sample run as partners. Make sure that they're designated as partners, as described in [Mark applications as partners](https://docs.microsoft.com/azure-sphere/app-development/sideload-app#mark-applications-as-partners), so that sideloading one doesn't delete the other.

If you're using Visual Studio or Visual Studio Code, you will need to deploy and debug both apps simutaneously. See the following instructions for building and running
the sample with Visual Studio or Visual Studio Code:

### Build and run the sample with Visual Studio

1. On the **File** menu, select **Open > Folder**.
1. Navigate to your Azure Sphere samples directory, select IntercoreComms, and click **Select Folder**.
1. On the **Select Startup Item** menu, select **GDB Debugger (All Cores)**.
1. On the **Build** menu, select **Build All**.
1. On the **Debug** menu, select **Start**, or press **F5**.

### Build and run the sample with Visual Studio Code

Use the [Visual Studio Code Multi-root Workspaces](https://code.visualstudio.com/docs/editor/multi-root-workspaces) feature to build and debug the RTApp and high-level app at the same time. 

1. On the **File** menu, **Select Open Workspace**.
1. Navigate to the IntercoreComms root directory and select the file *intercore.code-workspace*. 
1. Click **Open**.
1. After the build files have been created, right-click on either of the two *CMakeLists.txt* files and select **Build All Projects**.
1. Click the **Run** icon in the menu on the left side of the screen.
1. On the pulldown menu, that appears at the top of the window on the left side of the screen, select **Launch for azure Sphere Applications (gdb)(workspace)**.
1. On the **Run** menu, select **Start Debugging**. 

If you're running the sample from the command line you will need to build and run the RTApp before you build and run the high-level app. For more information about building real-time capable
and high-level applications from the command line, go to [Build a sample application](../../BUILD_INSTRUCTIONS.md) and click on the links "Tutorial: Build a real-time capable application" 
and "Build and run a high-level sample with the CLI" respectively.

## Observe the output

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
