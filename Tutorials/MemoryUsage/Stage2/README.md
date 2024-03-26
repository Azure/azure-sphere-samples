# Stage 2: Application is fixed

The goals of this stage are:

- To show the complete and correct implementation of an application to allocate and free user memory.
- To demonstrate tracking of shared library heap memory usage.

This application allocates and frees user memory as follows:

- When Button A is pressed the application allocates and inserts nodes to a linked list. Each node contains:
  - A user data buffer which is allocated on the heap.
  - A pointer to the next node.
- When Button B is pressed the application erases the last node and frees the memory allocated for the node.
- This application solves the memory leak introduced in Stage 1.

## Azure Sphere libraries

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|---------|---------|
| [EventLoop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages Button A and Button B on the device |
| [libcurl](https://learn.microsoft.com/azure-sphere/reference/baseapis) | Init the cURL library in order to demonstrate tracking of shared library heap memory usage |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging |

## Contents

| File/folder | Description |
|-------------|-------------|
| Stage2   | Tutorial source code and project files |
| README.md | This readme file |

## Prerequisites

The tutorial requires the following hardware:

* [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Setup

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Ensure that you have Azure Sphere SDK version 24.03 or above. At the command prompt, run `az sphere show-sdk-version` to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Ensure that the [Azure CLI](https://learn.microsoft.com/cli/azure/install-azure-cli) is installed. At a minimum, the Azure CLI version must be 2.45.0 or later.
1. Install the [Azure Sphere extension](https://learn.microsoft.com/azure-sphere/reference/cli/overview?view=azure-sphere-integrated).
1. Enable application development, if you have not already done so, by entering the `az sphere device enable-development` command in the [command prompt](https://learn.microsoft.com/azure-sphere/reference/cli/overview?view=azure-sphere-integrated).

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MemoryUsage tutorial.

1. [Add heap memory allocation tracking](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#add-heap-memory-allocation-tracking) to the sample.

## Build and run the sample

To build and run this sample in Visual Studio, follow the instructions to [build and deploy a high-level app without debugging](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=visual-studio#build-and-deploy-the-application-in-visual-studio-without-debugging). If you are using the CLI to build and run, follow the instructions in [Build a high-level app](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=cli).

## Observe the output in Visual Studio (Windows only)

1. After starting the application, open the [Visual Studio Performance Profiler](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#starting-the-memory-usage-profiler).

   The first chart, Azure Sphere Device Physical Memory, shows the total memory, user memory, and peak user memory usage, rounded to the nearest KiB. The first three data columns correspond to the Azure Sphere Device Physical Memory chart.

   The second chart, Azure Sphere Heap Usage, shows the total heap memory and the shared library heap memory usage, also in KiB. The last three data columns correspond to the Azure Sphere Heap Usage chart. The last column, Heap Usage By Caller, shows the heap memory usage for the application and the static libraries as well as the memory used by the shared libraries. Note the memory used by libcurl.

1. Press Button A and then immediately press Button B.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. Pressing Button B removes and frees the memory occupied by the last node of the list.

   In the Azure Sphere Device Physical Memory chart, the user memory usage and the peak memory usage increase. The user and peak user memory usage may not change after freeing allocated memory, as the C runtime does not necessarily return freed allocations to the OS. Pressing Button A immediately followed by pressing Button B should keep the memory usage constant.

   In the Azure Sphere Heap Usage chart, the total heap memory usage increases. Pressing Button A immediately followed by pressing Button B should show a decrease in the memory usage for the total heap, eventually going back to the initial value of the total heap memory usage of the newly started application.

   Stage 2 solves the memory leak introduced in Stage 1: the DeleteLastNode function frees the memory occupied by the node and the user data buffer. In Stage 1 the memory allocated for the node was freed, while the user data wasn't (DeleteLastNode function should have freed the user data before freeing the node).

   In main.c the lines which were freeing the memory in the DeleteLastNode and DeleteList functions were changed to call the DeleteNode function. The DeleteNode function frees both the user data and the memory occupied by the node, solving the memory leak from Stage 1.

   Pressing Button A and then Button B allocates and then frees the memory, so the overall usage remains constant. In Stage 1 the user memory would increase in both charts even if a node was removed from the list right after it was added. This is a good indication that the code had a memory leak.

## Observe the output from the CLI (Windows and Linux)

Use the following steps to monitor memory usage from the command prompt:

1. To view an initial baseline of memory usage, use the `az sphere device app show-memory-stats` command. The memory usage retrieved using the command line is displayed in bytes, not in KiB as it is in Visual Studio.

1. Press Button A and then immediately press Button B.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. Pressing Button B removes and frees the memory occupied by the last node of the list.

   Stage 2 solves the memory leak introduced in Stage 1: the DeleteLastNode function frees the memory occupied by the node and the user data buffer. In Stage 1 the memory allocated for the node was freed, while the user data wasn't (DeleteLastNode function should have freed the user data before freeing the node).

   In main.c the lines which were freeing the memory in the DeleteLastNode and DeleteList functions were changed to call the DeleteNode function. The DeleteNode function frees both the user data and the memory occupied by the node, solving the memory leak from Stage 1.

   Pressing Button A and then Button B allocates and then frees the memory, so the overall usage remains constant, as you should be able to verify with repeated calls to `az sphere device app show-memory-stats`.
