# Stage 1: Application has a memory leak

The goals of this stage are:

- To gain familiarity with the memory-usage reporting features of Azure Sphere using either [Visual Studio](#observe-the-output-in-visual-studio-windows-only) or the [Azure Sphere CLI](#observe-the-output-from-the-cli-windows-and-linux).
- To see the impact of a memory leak on the application's total memory usage. By pressing Button A and Button B repeatedly, you will see memory usage build in the application.
- To correct the memory leak and see improved behavior.
- To demonstrate tracking of shared library heap memory usage.

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
| [EventLoop](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages Button A and Button B on the device |
| [libcurl](https://learn.microsoft.com/azure-sphere/reference/baseapis) | Init the cURL library in order to demonstrate tracking of shared library heap memory usage |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the **Device Output** window during debugging |

## Contents

| File/folder | Description |
|-------------|-------------|
| Stage1	| Tutorial source code and project files |
| README.md | This readme file |

## Prerequisites

The tutorial requires the following hardware:

* [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.

**Note:** By default, this sample targets [MT3620 reference development board (RDB)](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studio. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the CMakeLists.txt file. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../../HardwareDefinitions/README.md).

## Setup

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, [ensure that you have Azure Sphere SDK version 23.05 or above](https://learn.microsoft.com/azure-sphere/reference/azsphere-show-version). At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the MemoryUsage tutorial.

1. Add [heap memory allocation tracking](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#add-heap-memory-allocation-tracking) to the sample:

   * Add the HeapMemStats capability to the sample app-manifest.json file.

      ```json
         "Capabilities": {
         "HeapMemStats": true
       },
      ```

   * Add the libmalloc library to the sample image package by replacing the command `azsphere_target_add_image_package(${PROJECT_NAME})` in the sample CMakeLists.txt file with the command `azsphere_target_add_image_package(${PROJECT_NAME} DEBUG_LIB "libmalloc"`. 
    
## Build and run the sample

To build and run this sample in Visual Studio, follow the instructions to [build and deploy a high-level app without debugging](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=visual-studio#build-and-deploy-the-application-in-visual-studio-without-debugging). If you are using the CLI to build and run, follow the instructions in [Build a high-level app](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=cli).

## Observe the output in Visual Studio (Windows only)

1. After starting the application, open the [Visual Studio Performance Profiler](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=visual-studio#starting-the-memory-usage-profiler).

   The first chart, Azure Sphere Device Physical Memory, shows the total memory, user memory, and peak user memory usage, rounded to the nearest KiB. The first three data columns correspond to the Azure Sphere Device Physical Memory chart.

   The second chart, Azure Sphere Heap Usage, shows the total heap memory and the shared library heap memory usage, also in KiB. The last three data columns correspond to the Azure Sphere Heap Usage chart. The last column, Heap Usage By Caller, shows the heap memory usage for the application and the static libraries as well as the memory used by the shared libraries. Note the memory used by libcurl.

1. Press Button A and then immediately press Button B.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. Pressing Button B removes and frees the memory occupied by the last node of the list.

   In the Azure Sphere Device Physical Memory chart, the user memory usage and the peak memory usage increase. The user and peak memory usage may not change after freeing allocated memory, as the C runtime does not necessarily return freed allocations to the OS. Pressing Button A immediately followed by pressing Button B should keep the memory usage constant.

   In the Azure Sphere Heap Usage chart, the total heap memory usage increases. Pressing Button A immediately followed by pressing Button B should show a decrease in the memory usage for the total heap, eventually going back to the initial value of the total heap memory usage of the newly started application.

   The memory usage profiler shows an increase in memory usage, even when pressing Button A is immediately followed by pressing Button B. Eventually, the application will exceed the memory limit and it will be terminated. If you are running without debugging, the application will then automatically restart. If you are running under the debugger, you will see a message beginning `Child terminated` in the Visual Studio output.

   The Visual Studio charts are a good indicator that the code has a memory leak.

1. Stop the application—as the application is not running under the Visual Studio debugger, you need to do this with the [Azure Sphere CLI **azsphere device app stop** command](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#app-stop).

1. Replace the call to `DeleteLastNode` inside the `CheckButtonDeleteLastNode` function in main.c with a call to `DeleteList`.

1. [Compile the application and deploy it on the device](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=visual-studio#build-and-deploy-the-application-in-visual-studio-without-debugging).

    Did the memory usage pattern change? Can you figure out which function leaks memory?

## Observe the output from the CLI (Windows and Linux)

Use the following steps to monitor memory usage from the Azure Sphere CLI:

1. To view an initial baseline of memory usage, use the [**azsphere device app show-memory-stats**](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#app-show-memory-stats) command:

   ```
   azsphere device app show-memory-stats
   ```
   The memory usage retrieved using the command line is displayed in bytes, not in KiB as it is in Visual Studio.

1. Press Button A and then immediately press Button B. Do this several times.

   Pressing Button A allocates and inserts a new node to the list. The node contains a buffer of 5,000 integers and a pointer to the next node of the list. Pressing Button B removes and frees the memory occupied by the last node of the list.

1. Use **azsphere device app show-memory-stats** again:

   ```
   azsphere device app show-memory-stats
   ```

   After several cycles of pressing Button A and pressing Button B, the memory usage has increased substantially. If you keep pressing Button A and Button B in succession, the application will eventually exceed the memory limit and it will be terminated. If you are running without debugging, the application will then automatically restart.

1. Stop the application using the [Azure Sphere CLI **azsphere device app stop** command](https://learn.microsoft.com/azure-sphere/reference/azsphere-device#app-stop).

1. Replace the call to `DeleteLastNode` inside the `CheckButtonDeleteLastNode` function in main.c with a call to `DeleteList`.

1. [Compile the application and deploy it on the device](https://learn.microsoft.com/azure-sphere/app-development/build-hl-app?tabs=windows%2Ccliv2beta&pivots=cli).

    Did the memory usage pattern change? Can you figure out which function leaks memory?

## Additional ideas on debugging a memory leak

- Override the native C memory allocation functions (malloc, realloc, calloc, alloc_aligned, and free), using the standard [GNU C Library](https://www.gnu.org/software/libc/manual/html_mono/libc.html) wrapping mechanism.
- Use the [HeapTracker library](https://github.com/Azure/azure-sphere-gallery/tree/main/HeapTracker) which is a thin-layer library that implements a custom heap tracking mechanism.
- [Learn more about memory constraints in high-level applications](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage).
- [Use loops to perform continuous monitoring of memory usage](https://learn.microsoft.com/azure-sphere/app-development/application-memory-usage?pivots=cli#continuous-monitoring-of-memory-usage).
- Add calls to the [<applibs/applications.h> functions](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-applications/applications-overview) to track the memory used by your program.

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
