# Hardware definitions

Azure Sphere hardware is available from multiple vendors, and each vendor may expose features of the underlying chip in different ways. Using hardware abstraction, the sample applications in the [Azure Sphere samples repository](https://github.com/Azure/azure-sphere-samples) on GitHub are hardware independent. They will run on any Azure Sphere-compatible hardware without changes to the code.

Hardware abstraction is achieved through the use of hardware definition files. The samples that require hardware definitions are written to run on an abstract *sample appliance*. For each supported board or module there is a hardware definition file called `sample_appliance.json`. This file maps the peripherals used by the sample appliance to corresponding peripherals on the board or module. The contents of this file are used during the build procedure to update the `app_manifest.json` file and in compiling and packaging the sample.

For additional information, see [Manage hardware dependencies](https://learn.microsoft.com/azure-sphere/app-development/manage-hardware-dependencies) and [Hardware abstraction files](https://learn.microsoft.com/azure-sphere/hardware/hardware-abstraction) in the online documentation.

**Note:** Hardware definitions are required only by high-level applications that use hardware peripherals. Therefore, not all samples use the hardware definitions feature.

## Supported hardware

A hardware definition file, `sample_appliance.json`, is provided for each of the following Azure Sphere development boards.

- [MT3620 reference development board (RDB)](mt3620_rdb/), which is the design used by the Seeed MT3620 development kit
- [Seeed MT3620 mini-dev board](seeed_mt3620_mdb/), which uses the AI-Link WF-M620-RSC1 module
- [Avnet MT3620 SK](avnet_mt3620_sk/), which uses the Avnet AES-MS-MT3620 module
- [USI MT3620 BT EVB](usi_mt3620_bt_evb/), which uses the USI USI-MT3620-BT-COMBO module

**Note:** The [Device to Cloud](https://learn.microsoft.com/samples/azure/azure-sphere-samples/devicetocloud/) reference solution has its own custom hardware definition files that support the RDB and the corresponding [hardware reference design](https://github.com/Azure/azure-sphere-hardware-designs/tree/main/P-MT3620EXMSTLP-1-0).

## Changing the target hardware for a sample application

The value of the TARGET_HARDWARE variable in a sample's `CMakeLists.txt` file specifies which development board is targeted by the sample. To set or change the target hardware for a sample application, complete the following steps:

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository or download the zip file for a sample from the [Microsoft samples browser](https://learn.microsoft.com/samples/browse/?products=azure-sphere) if you have not done so already.

   The hardware definition files for the sample appliance are in the `HardwareDefinitions` directory provided with the samples. Underlying hardware definition files, for example for the MT3620 RDB or the MT3620 chip itself, are [included in the SDK install folder](https://learn.microsoft.com/azure-sphere/hardware/hardware-abstraction).

1. Open the sample's `CMakeLists.txt` file, which is located in the sample's source directory.

1. In the `CMakeLists.txt` file, find the code statement that begins with `set(TARGET_HARDWARE`.

   **Note:** This code statement will not be present in the sample's `CMakeLists.txt` file if the sample has no hardware dependency or if the sample uses a custom hardware definition file.

1. Change the value of the `TARGET_HARDWARE` variable to match your hardware. For example, to run the sample on the Avnet MT3620 Starter Kit, the value of the `TARGET_HARDWARE` variable must be `avnet_mt3620_sk` as shown below:

    `set(TARGET_HARDWARE "avnet_mt3620_sk")`

## Using other hardware features from your dev board

The sample appliance header included with `#include <hw/sample_appliance.h>` recursively includes the underlying headers for your dev board. For example, if you are using the RDB, the constant `MT3620_RDB_LED4_RED` is defined and can be used in a call to `GPIO_OpenAsOutput()`. To discover the available constants, you can examine the header file by using Ctrl-Click in Visual Studio and VS Code to open a header file. Make sure that you update your `app_manifest.json` file to declare any new peripherals you are using. In the previous example, you would need to add `"$MT3620_RDB_LED4_RED"` to the Gpio list under the Capabilities section.

## Implementing custom hardware abstractions

Custom hardware definitions can be created so that your own code remains independent of specific hardware, and is easy to recompile for different hardware variants. For additional information, see [Manage hardware dependencies](https://learn.microsoft.com/azure-sphere/app-development/manage-hardware-dependencies) and [Hardware abstraction files](https://learn.microsoft.com/azure-sphere/hardware/hardware-abstraction) in the online documentation.
