# Stage 2: Application is fixed

The goal of this stage is to show the complete and correct implementation of an application to allocate and free user memory.

This application allocates and frees user memory as follows:

- When Button A is pressed the application allocates and inserts nodes to a linked list. Each node contains:
  - A user data buffer which is allocated on the heap.
  - A pointer to the next node.
- When Button B is pressed the application erases the last node and frees the memory allocated for the node.
- This application solves the memory leak introduced in Stage 1.

## Azure Sphere libraries

The tutorial uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages Button A and Button B on the device |

## Contents

| File/folder | Description |
|-------------|-------------|
| Stage2   | Tutorial source code and project files |
| README.md | This readme file |

## Prerequisites

The tutorial requires the following hardware:

* [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.02 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MemoryUsage tutorial.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md). This tutorial is intended to be built and run using Visual Studio.

## Observe the output

1. When starting the application, the upper-right corner of Visual Studio displays a [memory usage profiler](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#determine-run-time-application-memory-usage). The view shows the total memory, user memory, and peak memory usage.

1. Press Button A and then immediately press Button B.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. The user memory usage and the peak memory usage increase. Pressing Button B removes and frees the memory occupied by the last node of the list. The user and peak memory usage may not change after freeing the allocated memory, as the C runtime does not necessarily return freed allocations to the OS.

   Pressing Button A followed by a pressing Button B should keep the memory usage constant.

   Stage 2 solves the memory leak introduced in Stage 1: the DeleteLastNode function frees the memory occupied by the node and the user data buffer. In Stage 1 the memory allocated for the node was freed, while the user data wasn't (DeleteLastNode function should have freed the user data, before freeing the node).

   In main.c the lines which were freeing the memory in the DeleteLastNode and DeleteList functions were changed to call the DeleteNode function. The DeleteNode function frees both the user data and the memory occupied by the node, solving the memory leak from Stage 1.

   Pressing Button A and then Button B allocates and then frees the memory, so the overall usage remains constant. In Stage 1 the user memory would increase even if a node was removed from the list right after it was added. This was a good indication that the code had a memory leak.
