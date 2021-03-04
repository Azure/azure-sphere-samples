# Stage 1: Application has a memory leak

The goals of this stage are:

- To gain familiarity with the memory-usage reporting features of Azure Sphere and the Visual Studio extension.
- To see the impact of a memory leak on the application's total memory usage. By pressing Button A and Button B repeatedly, you will see memory usage build in the application.
- To correct the memory leak and see improved behavior.

This application allocates and frees user memory as follows:

- When Button A is pressed the application allocates and inserts nodes to a linked list. Each node contains:
  - A user data buffer which is allocated on the heap.
  - A pointer to the next node.
- When Button B is pressed the application erases the last node and frees the memory allocated for the node.
- This application has an intentional memory leak.

## Azure Sphere libraries

The tutorial uses the following Azure Sphere libraries:

| Library | Purpose |
|---------|---------|
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages Button A and Button B on the device |

## Contents

| File/folder | Description |
|-------------|-------------|
| Stage1	| Tutorial source code and project files |
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

   To get accurate memory usage values, [start the application without the debugger and start the performance profiler](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#starting-the-memory-usage-profiler).

1. Press Button A and then immediately press Button B.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. The user memory usage and the peak memory usage increase. Pressing Button B removes and frees the memory occupied by the last node of the list. The user and peak memory usage may not change after freeing allocated memory, as the C runtime does not necessarily return freed allocations to the OS. Pressing Button A immediately followed by pressing Button B should keep the memory usage constant.

   The memory usage profiler shows an increase in memory usage, even when pressing Button A is immediately followed by pressing Button B. Eventually, the application will exceed the memory limit and it will be terminated. If you are running without debugging, the application will then automatically restart. If you are running under the debugger, you will see a message beginning `Child terminated` in the Visual Studio output.

   The Visual Studio graph is a good indicator that the code has a memory leak.

1. Stop the application and replace the call to `DeleteLastNode` in main.c with a call to `DeleteList`.

1. Compile the application and deploy it on the device.

    Did the memory usage pattern change? Can you figure out which function leaks memory?

## Additional ideas on debugging a memory leak

- Override the native C memory allocation functions (malloc, realloc, calloc, alloc_aligned, and free), using the standard [GNU C Library](https://www.gnu.org/software/libc/manual/html_mono/libc.html) wrapping mechanism.
- Use the [HeapTracker library](https://github.com/Azure/azure-sphere-gallery/tree/main/HeapTracker) which is a thin-layer library that implements a custom heap tracking mechanism.
- [Learn more about memory constraints in high-level applications](https://docs.microsoft.com/azure-sphere/app-development/application-memory-usage).
- Add calls to the [<applibs/applications.h> functions](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-applications/applications-overview) to track the memory used by your program.

## Fix the application

1. The function which leaks memory is the `DeleteLastNode` function. In this function, the node is freed, but not the user data associated with it. To solve the memory leak, the user data must be freed before freeing the node. Copy and paste the following function into main.c (be sure to include the appropriate declaration for the function as well):

   ```C
   static void DeleteNode(Node *nodeToErase)
   {
      free(nodeToErase->userData);
      free(nodeToErase);
   }
   ```

   In the `DeleteLastNode` function, replace `free(*headNode)` with `DeleteNode(*headNode)` and `free(secondLast->next)` with `DeleteNode(secondLast->next)`. The full `DeleteLastNode` function then reads as follows:

   ```C
    /// <summary>
    ///     Erases the last node from the list.
    /// </summary>
    static void DeleteLastNode(Node **headNode)
    {
        if (*headNode == NULL) {
            Log_Debug("\nThe list is empty...\n");
            return;
        }

        listSize--;
        Log_Debug("\nDeleting the last node from the linked list (list size = %u).\n", listSize);

        // If the list contains just one node, delete it
        if ((*headNode)->next == NULL) {
            DeleteNode(*headNode);
            *headNode = NULL;
            return;
        }

        // Find the second last node
        Node *secondLast = *headNode;
        while (secondLast->next->next != NULL) {
            secondLast = secondLast->next;
        }

        DeleteNode(secondLast->next);

        secondLast->next = NULL;
    }
   ```

1. To see a complete, correct implementation of the code discussed in this tutorial, [proceed to Stage 2](../Stage2).
