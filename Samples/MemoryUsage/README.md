---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere - MemoryUsage
urlFragment: MemoryUsage
extendedZipContent:
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how to use the Applications APIs to gather information about the memory usage of the application."
---

# Sample: Memory Usage high-level app

This application does the following:

- Starts a timer which has the allocation/free memory logic
- Allocates a large buffer on the heap
- Allocates and appends nodes in a linked list
	- Each node contains:
		- an user data buffer which is allocated on the heap
		- a socket which is opened
		- a pointer to the next node
- When MAX_NUMBER_NODES are allocated, the application
	- deallocates the large buffer (if it's not NULL)
	- frees the memory occupied by the node buffer 
	- closes the node socket
	- erases the linked list
	- resets the node counter to 0
- Restarts allocating memory and appending nodes
	

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |
| [applications](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-application/applications-overview) | Acquire information about the memory usage |
## Contents

| File/folder | Description |
|-------------|-------------|
| MemoryUsage       |Sample source code and VS project files |
| README.md | This readme file |

## Prerequisites

The sample requires the following hardware:

* [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.04 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MemoryUsage sample.

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

## Observe the output

 The application outputs the user, peak and total memory.
 It starts by allocating a buffer of 4000 integers.
 It continues by appending nodes to a linked list.
 When it allocated MAX_NUMBER_NODES, it deallocates the buffer, deletes the list and restarts allocating and appending nodes.
 The user and peak memory usage may not change after freeing allocated memory, as the C runtime does not necessarily return freed allocations to the OS.
 When the list is deleted, the total memory usage decreases, because the sockets (which consume kernel memory) are closed.
 The application continues to append nodes to the linked list and then to free the memory.
